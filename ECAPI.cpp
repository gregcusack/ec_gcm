//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"

int ec::ECAPI::create_ec() {
    _ec = new ElasticContainer(ecapi_id);
    return 0;
}

const ec::ElasticContainer& ec::ECAPI::get_elastic_container() const {
    if(_ec == nullptr) {
        SPDLOG_CRITICAL("Must create _ec before accessing it");
        exit(EXIT_FAILURE);
    }
    return *_ec;
}

ec::ECAPI::~ECAPI() {
    delete _ec;
}

//// This is where we see the connection be initiated by a container on some node
int ec::ECAPI::handle_add_cgroup_to_ec(const ec::msg_t *req, ec::msg_t *res, const uint32_t ip, int fd) {
    if(!req || !res) {
        SPDLOG_ERROR("res or req == null in handle_add_cgroup_to_ec()");
        return __ALLOC_FAILED__;
    }

    //Check quota
    uint64_t quota;
    int update_quota = determine_quota_for_new_pod(req->rsrc_amnt, quota);


    auto *sc = _ec->create_new_sc(req->cgroup_id, ip, fd, quota, req->request); //update with throttle and quota
    if (!sc) {
        SPDLOG_ERROR("Unable to create new sc object");
        return __ALLOC_FAILED__;
    }

    //todo: possibly lock subcontainers map here
    int ret = _ec->insert_sc(*sc);

    //todo: Delete sc if ret == alloc_failed!
    _ec->update_fair_cpu_share();

    // And so once a subcontainer is created and added to the appropriate distributed container,
    // we can now create a map to link the container_id and agent_client
    SPDLOG_TRACE("Init. Added cgroup to _ec. cgroup id: {}", *sc->get_c_id());
    AgentClientDB* acdb = AgentClientDB::get_agent_client_db_instance();
    auto agent_ip = sc->get_c_id()->server_ip;
    auto target_agent = acdb->get_agent_client_by_ip(agent_ip);
    if ( target_agent ){
        std::lock_guard<std::mutex> lk(cv_mtx);
        _ec->add_to_sc_ac_map(*sc->get_c_id(), target_agent);
        SPDLOG_TRACE("handle() sc_id, agent_ip: {}, {}", *sc->get_c_id(), target_agent->get_agent_ip());
        cv.notify_one();
    } else {
        SPDLOG_ERROR("SubContainer's node IP or Agent IP not found!");
    }

    //Update pod quota
    if(update_quota) {
        int sys_ret = set_sc_quota_syscall(sc, quota, 13); // 13 seq number???
        if (sys_ret) {
            SPDLOG_ERROR("Can't read from socket to resize quota (on sc insert!). ret: {}", ret);
        }
    }

    res->request = 0; //giveback (or send back)
    return ret;
}

void ec::ECAPI::ec_decr_unalloc_memory_in_pages(uint64_t mem_to_reduce) {
    _ec->decr_unalloc_memory_in_pages(mem_to_reduce);
}

void ec::ECAPI::ec_incr_unalloc_memory_in_pages(uint64_t mem_to_incr) {
    _ec->incr_unalloc_memory_in_pages(mem_to_incr);
}

uint64_t ec::ECAPI::get_machine_free_memory(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
     // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
    AgentClient* ac = _ec->get_corres_agent(container_id);  
    if(!ac) {
        SPDLOG_ERROR("NO AgentClient found for container id: {}", container_id);
        return 0;
    }
//    ec::SubContainer sc = _ec->get_subcontainer_front(container_id);
    ret = ec::Facade::MonitorFacade::CAdvisor::getMachineFreeMem(ac->get_agent_ip().to_string());
    return ret;
}

int64_t ec::ECAPI::get_cpu_quota_in_us(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
    // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
    AgentClient* ac = _ec->get_corres_agent(container_id);
    if(!ac) {
        SPDLOG_ERROR("NO AgentClient found for container id: {}", container_id);
        return 0;
    }
//    ec::SubContainer sc = _ec->get_subcontainer_front(container_id);

//    if(sc.get_docker_id().empty()) {
//        SPDLOG_ERROR("docker_id is 0!");
//        return 0;
//    }
    if(container_id.docker_id.empty()) {
        SPDLOG_ERROR("docker_id is 0!");
        return 0;
    }
//    ret = ec::Facade::MonitorFacade::CAdvisor::getContCPUQuota(ac->get_agent_ip().to_string(), sc.get_docker_id());
    ret = ec::Facade::MonitorFacade::CAdvisor::getContCPUQuota(ac->get_agent_ip().to_string(), container_id.docker_id);
    return ret;
}


int64_t ec::ECAPI::set_sc_quota_syscall(ec::SubContainer *sc, uint64_t _quota, uint32_t seq_number) {
    if(!sc) {
        SPDLOG_CRITICAL("sc == NULL in manager set_sc_quota_syscall()");
        std::exit(EXIT_FAILURE);
    }

    auto diff_quota = (int64_t)_quota - (int64_t)sc->get_quota(); //new quota - old
    auto change = diff_quota < 0 ? "decr" : "incr";
//    SPDLOG_INFO("diff_quota, change: {}, {}", diff_quota, change);

    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(0); //__CPU__
    msg_req.set_cgroup_id(sc->get_c_id()->cgroup_id);
    msg_req.set_request(seq_number);
    msg_req.set_quota(_quota);
    msg_req.set_payload_string(change);
//    msg_req.set_payload_string(sc->get_docker_id());
//    SPDLOG_INFO("set_quota cgid: {}", *sc->get_c_id());

    while(unlikely(!sc->sc_inserted())) {
//        std::cout << "itr incr on sc inserted" << std::endl;
    }

    auto agent = _ec->get_corres_agent(*sc->get_c_id());
    if(!agent) {
        SPDLOG_CRITICAL("agent for container == NULL. cg_id: {}", *sc->get_c_id());
        std::exit(EXIT_FAILURE);
    }
    int64_t ret = agent->send_request(msg_req);
    return ret;

}

int64_t ec::ECAPI::get_sc_quota(ec::SubContainer *sc) {
    if(!sc) {
        SPDLOG_CRITICAL("sc == NULL in manager get_sc_quota()");
        std::exit(EXIT_FAILURE);
    }
    uint32_t seq_number = 1;
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(7); //__READ_QUOTA__
    msg_req.set_cgroup_id(sc->get_c_id()->cgroup_id);
    msg_req.set_request(seq_number);
    msg_req.set_payload_string("test");

    auto agent = _ec->get_corres_agent(*sc->get_c_id());
    if(!agent) {
        SPDLOG_CRITICAL("agent for container == NULL");
        std::exit(EXIT_FAILURE);
    }
    int64_t ret = agent->send_request(msg_req);
    return ret;
}

int64_t ec::ECAPI::sc_resize_memory_limit_in_pages(const ec::SubContainer::ContainerId& container_id, uint64_t new_mem_limit) {
    uint64_t ret = 0;
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(5); //RESIZE_MEM_LIMIT
    msg_req.set_cgroup_id(container_id.cgroup_id);
    msg_req.set_payload_string("test");
    msg_req.set_rsrc_amnt(new_mem_limit);

    auto agent = _ec->get_corres_agent(container_id);

    if(!agent) {
        SPDLOG_CRITICAL("temp is NULL");
        std::exit(EXIT_FAILURE);
    }
    
    ret = agent->send_request(msg_req);
    return ret;
}

int ec::ECAPI::determine_quota_for_new_pod(uint64_t req_quota, uint64_t &quota) {
    int update_quota_flag = 0;
    quota = req_quota;

    SPDLOG_TRACE("pod add input quota pre determine quota: {}", quota);
    if(quota <= ec_get_cpu_unallocated_rt()) {
        ec_decr_unallocated_rt(req_quota);
        ec_incr_alloc_rt(quota);
    }
    else if(!ec_get_cpu_unallocated_rt()) {
        quota = ec_get_cpu_slice();
        ec_incr_overrun(quota);
        update_quota_flag = 1;
        ec_incr_alloc_rt(quota);
    }
    else if(quota > ec_get_cpu_unallocated_rt()) {
        quota = ec_get_cpu_unallocated_rt();
        ec_set_unallocated_rt(0);
        update_quota_flag = 1;
        ec_incr_alloc_rt(quota);
    }

    SPDLOG_TRACE("pod add input quota post determine quota: {}", quota);
    return update_quota_flag;
}

void ec::ECAPI::ec_update_alloc_memory_in_pages(uint64_t mem) {
    ec_decr_unalloc_memory_in_pages(mem);
    ec_incr_alloc_memory_in_pages(mem);
}

void ec::ECAPI::ec_update_reclaim_memory_in_pages(uint64_t mem) {
    ec_incr_unalloc_memory_in_pages(mem);
    ec_decr_alloc_memory_in_pages(mem);
}

void ec::ECAPI::sc_set_memory_limit_in_pages(ec::SubContainer::ContainerId sc_id, uint64_t new_mem_limit) {
    auto sc = ec_get_sc_for_update(sc_id);
    sc->set_mem_limit_in_pages(new_mem_limit);
}

uint64_t ec::ECAPI::__syscall_get_memory_usage_in_bytes(const ec::SubContainer::ContainerId &sc_id) {
    uint64_t ret = 0;
    //initialize request body
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(6); //MEM_usage_IN_PAGES 
    msg_req.set_cgroup_id(sc_id.cgroup_id);
    msg_req.set_payload_string("test");
    //std::cerr << "[dbg] get_memory_usage_in_bytes: get the corresponding agent\n";
    auto agent = _ec->get_corres_agent(sc_id);
    if(!agent) {
        std::cerr << "[dbg] agent is NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    ret = agent->send_request(msg_req);
    SPDLOG_TRACE("memory usage in pages reported from the agent: {}", ret);
    return ret * __PAGE_SIZE__ ;
}

uint64_t ec::ECAPI::__syscall_get_memory_limit_in_bytes(const ec::SubContainer::ContainerId &sc_id) {
    uint64_t ret = 0;
    //initialize request body
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(7); //MEM_limit_IN_PAGES 
    msg_req.set_cgroup_id(sc_id.cgroup_id);
    msg_req.set_payload_string("test");
//    std::cerr << "[dbg] __syscall_get_memory_limit_in_bytes: get the corresponding agent\n";
    auto agent = _ec->get_corres_agent(sc_id);
    if(!agent) {
        std::cerr << "[dbg] agent is NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    ret = agent->send_request(msg_req);
    return ret * __PAGE_SIZE__ ;
}

