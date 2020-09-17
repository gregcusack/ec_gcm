//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"

//ec::ECAPI::ECAPI(uint32_t _ec_id)
//    : manager_id(_ec_id) {
 //   // _ec = new ElasticContainer(manager_id, agent_clients);
//}

int ec::ECAPI::create_ec() {
//    _ec = new ElasticContainer(manager_id);
    _ec = new ElasticContainer(ecapi_id);
//    grpcServer = new rpc::DeployerExportServiceImpl(_ec, cv, cv_mtx, sc_map_lock);
    return 0;
}

const ec::ElasticContainer& ec::ECAPI::get_elastic_container() const {
    if(_ec == nullptr) {
        std::cout << "[ERROR]: Must create _ec before accessing it" << std::endl;
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
        std::cout << "ERROR. res or req == null in handle_add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }

    //Check quota
    uint64_t quota;
    int update_quota = determine_quota_for_new_pod(req->rsrc_amnt, quota);


    auto *sc = _ec->create_new_sc(req->cgroup_id, ip, fd, quota, req->request); //update with throttle and quota
    if (!sc) {
        std::cerr << "[ERROR] Unable to create new sc object: Line 147" << std::endl;
        return __ALLOC_FAILED__;
    }

    //todo: possibly lock subcontainers map here
    int ret = _ec->insert_sc(*sc);

    //todo: Delete sc if ret == alloc_failed!
//    _ec->incr_total_cpu(sc->get_quota());
    _ec->update_fair_cpu_share();
//    std::cout << "fair share: " << ec_get_fair_cpu_share() << std::endl;

//    auto mem = sc_get_memory_limit_in_bytes(*sc->get_c_id());
//    ec_incr_memory_limit_in_pages(mem);

    // And so once a subcontainer is created and added to the appropriate distributed container,
    // we can now create a map to link the container_id and agent_client
    
//    std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_c_id() << std::endl;
    AgentClientDB* acdb = AgentClientDB::get_agent_client_db_instance();
    auto agent_ip = sc->get_c_id()->server_ip;
    auto target_agent = acdb->get_agent_client_by_ip(agent_ip);
//    std::cout << "[dbg] Agent client ip: " << target_agent-> get_agent_ip() << std::endl;
//    std::cout << "[dbg] Agent ip: " << agent_ip << std::endl;
    if ( target_agent ){
//        mtx.lock();
//        std::cout << "add to sc_ac map" << std::endl;
        std::lock_guard<std::mutex> lk(cv_mtx);
        _ec->add_to_sc_ac_map(*sc->get_c_id(), target_agent);
//        std::cout << "handle() sc_id, agent_ip: " << *sc->get_c_id() << ", " << target_agent->get_agent_ip() << std::endl;
        cv.notify_one();
//        mtx.unlock();
    } else {
        std::cerr<< "[ERROR] SubContainer's node IP or Agent IP not found!" << std::endl;
    }

    //Update pod quota
    if(update_quota) {
        int sys_ret = set_sc_quota_syscall(sc, quota, 13); // 13 seq number???
        if (sys_ret) {
            std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (on sc insert!). ret: " << ret
                      << std::endl;
        }
    }


//    std::cout << "returning from handle_Add_cgroup_to_ec(): ret: " << ret << std::endl;
    res->request = 0; //giveback (or send back)
    return ret;
}

void ec::ECAPI::ec_decr_unalloc_memory_in_pages(uint64_t mem_to_reduce) {
    _ec->decr_unalloc_memory_in_pages(mem_to_reduce);
}

void ec::ECAPI::ec_incr_unalloc_memory_in_pages(uint64_t mem_to_incr) {
    _ec->incr_unalloc_memory_in_pages(mem_to_incr);
}


uint64_t ec::ECAPI::sc_get_memory_limit_in_bytes(const ec::SubContainer::ContainerId &sc_id) {
    return _ec->get_sc_memory_limit_in_bytes(sc_id);
}

uint64_t ec::ECAPI::sc_get_memory_usage_in_bytes(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
    AgentClient* ac = _ec->get_corres_agent(container_id);
    if(!ac) {
        std::cerr << "[ERROR] NO AgentClient found for container id: " << container_id << std::endl;
        return 0;
    }
    ec::SubContainer sc = _ec->get_subcontainer(container_id);

//    std::cout << "docker id used:" <<  sc.get_docker_id() << std::endl;
//    std::cout << "docker id used:" <<  sc.get_docker_id() << std::endl;
    if(sc.get_docker_id().empty()) {
        std::cout << "docker_id is 0!" << std::endl;
        return 0;
    }
    ret = ec::Facade::MonitorFacade::CAdvisor::getContMemUsage(ac->get_agent_ip().to_string(), sc.get_docker_id());
    return ret;
}

uint64_t ec::ECAPI::get_machine_free_memory(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
     // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
    AgentClient* ac = _ec->get_corres_agent(container_id);  
    if(!ac) {
        std::cerr << "[ERROR] NO AgentClient found for container id: " << container_id << std::endl;
        return 0;
    }
    ec::SubContainer sc = _ec->get_subcontainer(container_id);
    ret = ec::Facade::MonitorFacade::CAdvisor::getMachineFreeMem(ac->get_agent_ip().to_string());
    return ret;
}

int64_t ec::ECAPI::get_cpu_quota_in_us(const ec::SubContainer::ContainerId &container_id) {
    uint64_t ret = 0;
    // This is where we'll use cAdvisor instead of the agent comm to get the mem limit
    AgentClient* ac = _ec->get_corres_agent(container_id);
    if(!ac) {
        std::cerr << "[ERROR] NO AgentClient found for container id: " << container_id << std::endl;
        return 0;
    }
    ec::SubContainer sc = _ec->get_subcontainer(container_id);

//    std::cout << "docker id used:" <<  sc.get_docker_id() << std::endl;
    if(sc.get_docker_id().empty()) {
        std::cout << "docker_id is 0!" << std::endl;
        return 0;
    }
    ret = ec::Facade::MonitorFacade::CAdvisor::getContCPUQuota(ac->get_agent_ip().to_string(), sc.get_docker_id());
    return ret;
}


int64_t ec::ECAPI::set_sc_quota_syscall(ec::SubContainer *sc, uint64_t _quota, uint32_t seq_number) {
    if(!sc) {
        std::cout << "sc == NULL in manager set_sc_quota_syscall()" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(0); //__CPU__
    msg_req.set_cgroup_id(sc->get_c_id()->cgroup_id);
    msg_req.set_request(seq_number);
    msg_req.set_quota(_quota);
    msg_req.set_payload_string("test");

//    std::cout << "updateing quota to (input, in msg_Req): (" << _quota << ", " << msg_req.quota() << ")" << std::endl;
    auto agent = _ec->get_corres_agent(*sc->get_c_id());
    if(!agent) {
        std::cerr << "agent for container == NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    int64_t ret = agent->send_request(msg_req);
    return ret;

}

int64_t ec::ECAPI::get_sc_quota(ec::SubContainer *sc) {
    if(!sc) {
        std::cout << "sc == NULL in manager get_sc_quota()" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    uint32_t seq_number = 1;
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(7); //__READ_QUOTA__
    msg_req.set_cgroup_id(sc->get_c_id()->cgroup_id);
    msg_req.set_request(seq_number);
    msg_req.set_payload_string("test");

//    std::cout << "updateing quota to (input, in msg_Req): (" << _quota << ", " << msg_req.quota() << ")" << std::endl;
    auto agent = _ec->get_corres_agent(*sc->get_c_id());
    if(!agent) {
        std::cerr << "agent for container == NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    int64_t ret = agent->send_request(msg_req);
    return ret;
}


int64_t ec::ECAPI::sc_resize_memory_limit_in_pages(ec::SubContainer::ContainerId container_id, uint64_t new_mem_limit) {
    uint64_t ret = 0;
    msg_struct::ECMessage msg_req;
    msg_req.set_req_type(5); //RESIZE_MEM_LIMIT
    msg_req.set_cgroup_id(container_id.cgroup_id);
    msg_req.set_payload_string("test");
    msg_req.set_rsrc_amnt(new_mem_limit);

    auto agent = _ec->get_corres_agent(container_id);

    if(!agent) {
        std::cerr << "[dbg] temp is NULL" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    ret = agent->send_request(msg_req);
    return ret;
}

int ec::ECAPI::determine_quota_for_new_pod(uint64_t req_quota, uint64_t &quota) {
    int update_quota_flag = 0;
    quota = req_quota;
    std::cout << "pod add input quota pre determine quota: " << quota << std::endl;
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
//    std::cout << "quota: " << quota << std::endl;
//    std::cout << "rsrc_amnt: " << req_quota << std::endl;
    std::cout << "pod add input quota post determine quota: " << quota << std::endl;
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



