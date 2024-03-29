//
// Created by Greg Cusack on 11/5/19.
//

#include "Manager.h"

ec::Manager::Manager(int _manager_id, ec::ip4_addr gcm_ip, ec::ports_t controller_ports, std::vector<Agent *> &agents)
        : Server(_manager_id, gcm_ip, controller_ports, agents), ECAPI(_manager_id), manager_id(_manager_id),
          syscall_sequence_number(0), cpuleak(0), deploy_service_ip(gcm_ip.to_string()), grpcServer(nullptr) {
    grpc_port = BASE_GRPC_PORT + _manager_id - 1; //4447 for manager_id 1, 4448 for manager_id, etc

    //init server
    initialize_tcp();
    initialize_udp();
}


void ec::Manager::start(const std::string &app_name,  const std::string &gcm_ip, const int num_containers) {
    //A thread to listen for subcontainers' events
    std::thread event_handler_thread_tcp(&ec::Server::serve_tcp, this);
    std::thread event_handler_thread_udp(&ec::Server::serve_udp, this);
    ec::ECAPI::create_ec();
    grpcServer = new rpc::DeployerExportServiceImpl(_ec, cv, cv_dock, cv_mtx, cv_mtx_dock,sc_map_lock);
    std::thread grpc_handler_thread(&ec::Manager::serveGrpcDeployExport, this);
    std::thread idle_pod_handler(&ec::Manager::check_for_idle_containers, this);

    sleep(30);

    std::cerr<<"[dbg] manager::just before running the app thread\n";
    std::thread application_thread(&ec::Manager::run, this, num_containers);
    
    grpc_handler_thread.join();
    event_handler_thread_tcp.join();
    event_handler_thread_udp.join();
    application_thread.join();

    delete getGrpcServer();
}

// Check for idle containers and reclaim memory from them
void ec::Manager::check_for_idle_containers() {
    uint64_t min_quota = 30000000;
    uint32_t idle_container_syscall_seq_num = 0;

    while(true) {
        auto now = std::chrono::high_resolution_clock::now();
        auto sc_map = _ec->get_subcontainers_map_for_update();
        for(auto &[sc_id, sc] : *sc_map) {
            auto idle = sc->back()->check_if_idle(now);
            if(idle) {
                uint64_t idle_quota = sc->back()->get_quota();
                std::cout << "sc_id in idle check. (sc_id, idle?) (" << sc_id << ", " << idle << ")" << std::endl;
                std::cout << "container quota: " << idle_quota / 1000 / 1000 << "% of core" << std::endl;
                if (idle_quota != min_quota) { // only update containers that have > 30% of core
                    int ret = set_sc_quota_syscall(sc->back(), min_quota,
                                                   idle_container_syscall_seq_num); //give back what was used + 5ms
                    if (ret) {
                        SPDLOG_ERROR("Can't read from socket to resize quota (decr). ret: {}", ret);
                    } else {
                        sc->back()->set_quota_flag(true);
                        ec_incr_unallocated_rt(idle_quota - min_quota); //unalloc_rt <-- old quota - new quota
                        sc->back()->set_quota(min_quota);
                        sc->back()->get_cpu_stats()->flush();
                        ec_decr_alloc_rt(idle_quota - min_quota);
                    }
                    idle_container_syscall_seq_num++;
                }
            }
        }
        std::cout << "------------------" << std::endl;
        sleep(2);
    }
}

int ec::Manager::handle_cpu_usage_report(const ec::msg_t *req, ec::msg_t *res) {
    if (req == nullptr || res == nullptr) {
        SPDLOG_CRITICAL("req or res == null in handle_cpu_usage_report()");
        exit(EXIT_FAILURE);
    }
    if (req->req_type != _CPU_) { return __ALLOC_FAILED__; }

//    cpulock.lock();
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    bool idle_flag = false;
    if (!sc) {
        SPDLOG_ERROR("sc is NULL!");
        return __ALLOC_SUCCESS__;
    }
    auto now = std::chrono::high_resolution_clock::now();
    if(sc->check_if_idle(now)) {
        idle_flag = true;
    }
    sc->update_last_seen_ts(now);
//    std::cout << "rx update from sc_id: " << sc_id << std::endl;

    sc->incr_cpustat_seq_num();
    auto rx_cpustat_seq_num = req->cpustat_seq_num;
    auto rx_quota = req->rsrc_amnt;
    auto rt_remaining = req->runtime_remaining;
    auto throttled = req->request;
    int ret;
    double thr_mean = 0;
    uint64_t total_rt_in_sys, rt_mean, to_add, updated_quota;
    uint32_t syscall_seq_num = syscall_sequence_number;

    if(sc->get_seq_num() != rx_cpustat_seq_num) {
        SPDLOG_ERROR("seq nums do not match for cg_id: ({}, {}), (rx, sc->get): ({}, {})",
                     sc->get_c_id()->server_ip, sc->get_c_id()->cgroup_id, rx_cpustat_seq_num, sc->get_seq_num());
        sc->set_cpustat_seq_num(rx_cpustat_seq_num);
    }

    if(rx_quota / 1000 != sc->get_quota() / 1000) {
        SPDLOG_ERROR("quotas do not match (ip, cgid, rx, sc->get): ({}, {}, {}, {})", sc->get_c_id()->server_ip, sc->get_c_id()->cgroup_id, rx_quota, sc->get_quota());
        if(sc->incr_quota_mismatch_counter() > 2) { //mistmatch stuck
            if(rx_quota < sc->get_quota()) {        //didn't set incr quota properly
                auto diff = sc->get_quota() - rx_quota;
                sc->set_quota(rx_quota);
                sc->get_cpu_stats()->flush();
                ec_decr_alloc_rt(diff);
                ec_incr_unallocated_rt(diff);
            } else {                                //rx_quota > sc->get_quota -> didn't set decr quota properly
                auto diff = rx_quota - sc->get_quota();
                ec_decr_unallocated_rt(diff);
                sc->set_quota(rx_quota);
                sc->get_cpu_stats()->flush();
                ec_incr_alloc_rt(diff);
            }
            sc->reset_quota_mismatch_counter();

        } else {
            res->request = 1;
            return __ALLOC_SUCCESS__;
        }
    }

    total_rt_in_sys = ec_get_alloc_rt() + ec_get_cpu_unallocated_rt(); //alloc, overrun, unalloc
    cpuleak += (int64_t)ec_get_total_cpu() - (int64_t)total_rt_in_sys;

    if( cpuleak >= _MAX_CPU_LOSS_IN_NS_) {
        SPDLOG_DEBUG("fix rt leak: tot cpuleak: {}", cpuleak);
        ec_incr_unallocated_rt(cpuleak);
        cpuleak = 0;
    }

    rt_mean = sc->get_cpu_stats()->insert_rt_stats(rt_remaining);
    thr_mean = sc->get_cpu_stats()->insert_th_stats(throttled);
    auto percent_decr = (double)((int64_t)rx_quota-((int64_t)rx_quota - (int64_t)rt_remaining)) / (double)rx_quota;

    if(idle_flag && ec_get_cpu_unallocated_rt() > 0) {
        uint64_t one_core = 100000000;         //1 core
        uint64_t new_quota = std::min(ec_get_cpu_unallocated_rt(), one_core);
        auto extra_rt = new_quota - rx_quota;
        if(extra_rt > 0) {
            ret = set_sc_quota_syscall(sc, new_quota, syscall_seq_num);
            if(ret) {
                SPDLOG_ERROR("Can't read from socket to resize quota (incr). ret: {}", ret);
            }
            else {
                sc->set_quota_flag(true);
                ec_decr_unallocated_rt(extra_rt);
                sc->set_quota(new_quota);
                sc->get_cpu_stats()->flush();
                ec_incr_alloc_rt(extra_rt);
            }
        }
    }
    else if(ec_get_overrun() > 0 && rx_quota > ec_get_fair_cpu_share()) {
        uint64_t to_sub;
        uint64_t amnt_share_over = rx_quota - ec_get_fair_cpu_share();
        uint64_t overrun = ec_get_overrun();
        double percent_over = ((double)rx_quota - (double)ec_get_fair_cpu_share()) / (double)ec_get_fair_cpu_share();
        if(percent_over > 1) {
            to_sub = (uint64_t) (percent_over * (double) ec_get_cpu_slice());
        }
        else {
            to_sub = (uint64_t) (percent_over * (double)amnt_share_over);
        }
        //TODO: thr_mean is probably important. Take less from containers that are constantly being throttled
        if(to_sub < ec_get_cpu_slice() / 2) { //ensures we eventually converge
            to_sub = amnt_share_over;
        }
        to_sub = std::min(overrun, to_sub);
        updated_quota = rx_quota - to_sub;
        ret = set_sc_quota_syscall(sc, updated_quota, syscall_seq_num);
        if(ret) {
            SPDLOG_ERROR("Can't read from socket to resize quota (overrun sub quota). ret: {}", ret);
        }
        else {
            sc->set_quota_flag(true);
            sc->get_cpu_stats()->flush();
            ec_decr_overrun(to_sub);
            sc->set_quota(updated_quota);
            ec_decr_alloc_rt(to_sub);
        }
    }
    else if(rx_quota < ec_get_fair_cpu_share() && thr_mean > 0.5) {   //throttled but don't have fair share
        uint64_t amnt_share_lacking = ec_get_fair_cpu_share() - rx_quota;
        if (ec_get_cpu_unallocated_rt() > 0) {
            //TODO: take min of to_Add and slice. don't full reset
            double percent_under = ((double)ec_get_fair_cpu_share() - (double)rx_quota) / (double)ec_get_fair_cpu_share();
            if(amnt_share_lacking > ec_get_cpu_slice() / 2) {   //ensure we eventually converge
                to_add = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(percent_under * (double)amnt_share_lacking));
            }
            else {
                to_add = amnt_share_lacking;
            }
            updated_quota = rx_quota + to_add;
            ret = set_sc_quota_syscall(sc, updated_quota, syscall_seq_num);
            if(ret) {
                SPDLOG_ERROR("Can't read from socket to resize quota (incr fair share). ret: {}", ret);
            }
            else {
                sc->set_quota_flag(true);
                ec_decr_unallocated_rt(to_add);
                sc->set_quota(updated_quota);
                sc->get_cpu_stats()->flush();
                ec_incr_alloc_rt(to_add);
            }
        }
        else { //not enough in unalloc_rt to get back to fair share, even out
            uint64_t overrun;
            double percent_under = ((double)ec_get_fair_cpu_share() - (double)rx_quota) / (double)ec_get_fair_cpu_share();
            if(amnt_share_lacking > ec_get_cpu_slice() / 2) { //ensure we eventualyl converge
                overrun = (uint64_t)((double) percent_under * amnt_share_lacking);
            }
            else {
                overrun = amnt_share_lacking;
            }
            updated_quota = rx_quota + overrun;
            ret = set_sc_quota_syscall(sc, updated_quota, syscall_seq_num);
            if(ret) {
                SPDLOG_ERROR("Can't read from socket to resize quota (incr fair share overrun). ret: {}", ret);
            }
            else {
                sc->set_quota_flag(true);
                ec_incr_overrun(overrun);
                sc->set_quota(updated_quota);
                sc->get_cpu_stats()->flush();
                ec_incr_alloc_rt(overrun);
            }
        }
    }
    else if(thr_mean >= 0.1 && ec_get_cpu_unallocated_rt() > 0) {  //sc_quota > fair share and container got throttled during the last period. need rt
        auto extra_rt = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(20 * thr_mean * ec_get_cpu_slice()));
        if(extra_rt > 0) {
            updated_quota = rx_quota + extra_rt;
            ret = set_sc_quota_syscall(sc, updated_quota, syscall_seq_num);
            if(ret) {
                SPDLOG_ERROR("Can't read from socket to resize quota (incr). ret: {}", ret);
            }
            else {
                sc->set_quota_flag(true);
                ec_decr_unallocated_rt(extra_rt);
                sc->set_quota(updated_quota);
                sc->get_cpu_stats()->flush();
                ec_incr_alloc_rt(extra_rt);
            }
        }
    }
    else if(percent_decr > 0.2 && rx_quota >= ec_get_cpu_slice()) { //greater than 20% of quota unused
        uint64_t new_quota = rx_quota * (1 - 0.2); //sc_quota - sc_rt_remaining + ec_get_cpu_slice();
        new_quota = std::max(ec_get_cpu_slice(), new_quota);
        if(new_quota != rx_quota) {
            ret = set_sc_quota_syscall(sc, new_quota, syscall_seq_num); //give back what was used + 5ms
            if(ret) {
                SPDLOG_ERROR("Can't read from socket to resize quota (decr). ret: {}", ret);
            }
            else {
                sc->set_quota_flag(true);
                ec_incr_unallocated_rt(rx_quota - new_quota); //unalloc_rt <-- old quota - new quota
                sc->set_quota(new_quota);
                sc->get_cpu_stats()->flush();
                ec_decr_alloc_rt(rx_quota - new_quota);
            }
        }
    }
    else if(rt_mean > _MAX_UNUSED_RT_IN_NS_ && rx_quota > ec_get_cpu_slice()) {
        uint64_t new_quota = rx_quota - _MAX_UNUSED_RT_IN_NS_;
        new_quota = std::max(ec_get_cpu_slice(), new_quota);
        if(new_quota != rx_quota) {
            ret = set_sc_quota_syscall(sc, new_quota, syscall_seq_num); //give back what was used + 5ms
            if(ret) {
                SPDLOG_ERROR("Can't read from socket to resize quota (decr 5ms diff). ret: {}", ret);
            }
            else {
                sc->set_quota_flag(true);
                ec_incr_unallocated_rt(rx_quota - new_quota); //unalloc_rt <-- old quota - new quota
                sc->set_quota(new_quota);
                sc->get_cpu_stats()->flush();
                ec_decr_alloc_rt(rx_quota - new_quota);
            }
        }
    }

    syscall_sequence_number++;
    res->request = 1;
//    cpulock.unlock();
    return __ALLOC_SUCCESS__;

}

int ec::Manager::handle_mem_req(const ec::msg_t *req, ec::msg_t *res, int clifd) {
    if(req == nullptr || res == nullptr) {
        SPDLOG_CRITICAL("req or res == null in handle_mem_req()");
        exit(EXIT_FAILURE);
    }
    SPDLOG_INFO("handle_mem_req()");
    uint64_t ret = 0;
    if(req->req_type != _MEM_) { res->rsrc_amnt = 0; return __ALLOC_MEM_FAILED__; }
    memlock.lock();
    int64_t memory_available = ec_get_unalloc_memory_in_pages();
    if(memory_available > 0) {
        SPDLOG_DEBUG("gcm memory avail > 0. Returning global slice. mem avail: {}", memory_available);
    }
    else {
        auto reclaimed_mem = handle_reclaim_memory(clifd);
        if(reclaimed_mem > 0) {
            if(memory_available < 0) {
                SPDLOG_CRITICAL("reclaiming memory and mem_available < 0. THis should not happen!!");
                SPDLOG_CRITICAL("----> mem_avail: {}, reclaimed_mem: {}", memory_available, reclaimed_mem);
                SPDLOG_CRITICAL("----> alloc_mem: {}, appl_mem_limit: {}", ec_get_alloc_memory_in_pages(), ec_get_mem_limit_in_pages());
                //SETTING MEM_AVAIL TO 0 AND GOING FROM THERE
                ec_incr_unalloc_memory_in_pages(memory_available * -1);
                ec_decr_alloc_memory_in_pages(memory_available * -1);
            }
            ec_update_reclaim_memory_in_pages(reclaimed_mem);
            memory_available = ec_get_unalloc_memory_in_pages();
            SPDLOG_DEBUG("reclaimed mem from other pods. mem rx: {}", memory_available);
        }
        else {
            memlock.unlock();
            SPDLOG_INFO("no memory available!");
            res->rsrc_amnt = 0;
            return __ALLOC_MEM_FAILED__;
        }

    }

    SPDLOG_DEBUG("Handle mem req: success. memory available: {}", memory_available);
    ret = memory_available > ec_get_memory_slice() ? ec_get_memory_slice() : memory_available;

    SPDLOG_DEBUG("mem amnt to ret: {}", ret);

    ec_update_alloc_memory_in_pages(ret);

    SPDLOG_DEBUG("successfully decrease unallocated mem to: {}", ec_get_unalloc_memory_in_pages());
    SPDLOG_DEBUG("successfully increased allocated mem to: {}", ec_get_alloc_memory_in_pages());


    res->rsrc_amnt = req->rsrc_amnt + ret;   //give back "ret" pages
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    sc->set_mem_limit_in_pages(res->rsrc_amnt);

    SPDLOG_TRACE("pre mem unlock: {}", SubContainer::ContainerId(req->cgroup_id, req->client_ip));
    memlock.unlock();
    res->request = 0;       //give back
    SPDLOG_TRACE("returning mem back to container. sc_id: {}", SubContainer::ContainerId(req->cgroup_id, req->client_ip));
    return __ALLOC_SUCCESS__;
}

uint64_t ec::Manager::reclaim(const SubContainer::ContainerId& containerId, SubContainer* subContainer){

    SPDLOG_TRACE("docker id: {}, {}, {}", containerId.docker_id, subContainer->get_docker_id(), subContainer->get_c_id()->docker_id);
	uint64_t pages_reclaimed = 0;
	auto mem_limit_pages = subContainer->get_mem_limit_in_pages();
    auto mem_limit_bytes = page_to_byte(mem_limit_pages);
    auto mem_usage_bytes = sc_get_memory_usage_in_bytes_cadvisor(containerId, subContainer->get_docker_id());

    if(mem_usage_bytes == (unsigned long)-1 * __PAGE_SIZE__) {
        SPDLOG_ERROR("failed to read mem usage for cg_id: {}", containerId);
        return pages_reclaimed;
    }

    SPDLOG_DEBUG("pre reclaim. mem usage, limit (bytes): {}, {}", mem_usage_bytes, mem_limit_bytes);

    if(mem_limit_bytes - mem_usage_bytes > _SAFE_MARGIN_BYTES_) {
        auto resize_target_bytes = mem_usage_bytes + _SAFE_MARGIN_BYTES_;

        SPDLOG_DEBUG("limit - usage > safe margin. going to resize max mem of cgid: {} to {} pages",
                     containerId, byte_to_page(resize_target_bytes));
        auto is_max_mem_resized = sc_resize_memory_limit_in_pages(containerId,
                                                                  byte_to_page(resize_target_bytes));
        if(!is_max_mem_resized) {
            SPDLOG_DEBUG("max mem WAS resized");
            pages_reclaimed = byte_to_page(mem_limit_bytes - (resize_target_bytes));
            sc_set_memory_limit_in_pages(*subContainer->get_c_id(), byte_to_page(resize_target_bytes));
        } else {
            SPDLOG_INFO("resizing of max mem failed!");
        }
        SPDLOG_DEBUG("mem limit resized from -> to: {} -> {} (bytes)", mem_limit_bytes, resize_target_bytes);
        SPDLOG_TRACE("mem limit resized from -> to: {} -> {} (pages)", byte_to_page(mem_limit_bytes), byte_to_page(resize_target_bytes));
        SPDLOG_TRACE("limit - (usage + safe margin) (bytes): {}", mem_limit_bytes - (resize_target_bytes));
    } else {
        SPDLOG_DEBUG("mem usage too close to mem_limit_bytes to resize! --> limit - usage: {}", mem_limit_bytes - mem_usage_bytes);
        SPDLOG_DEBUG("safe margin: {}", _SAFE_MARGIN_BYTES_);
    }
	return pages_reclaimed;

}

//todo: reclaim memory should already know what each container mem limit is
uint64_t ec::Manager::handle_reclaim_memory(int client_fd) {
	std::vector<std::future<uint64_t>> futures;
    std::vector<uint64_t> reclaim_amounts;

    SPDLOG_DEBUG("GCM: Trying to reclaim memory from other cgroups!");
    for (const auto &sc_map : ec_get_subcontainers()) {
        if (sc_map.second->back()->get_fd() == client_fd) {
            continue;
        }
        std::future<uint64_t> reclaimed = std::async(std::launch::async, &ec::Manager::reclaim, this, sc_map.first, sc_map.second->back());
        futures.push_back(std::move(reclaimed));
    }

    uint64_t ret = 0;
    for(auto &rec : futures) {
        ret += rec.get();   //TODO: get blocks. do we need a timeout if reclaim() never returns??
    }

    SPDLOG_DEBUG("Recalimed memory at the end of the reclaim function: {}", ret);
    return ret;
}

int ec::Manager::handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd){
    if(!req || !res) {
        SPDLOG_CRITICAL("req or res == null in handle_req()"); //todo: we shouldn't exit_failure on one null req
        exit(EXIT_FAILURE);
    }
    uint64_t ret = __FAILED__;

    SPDLOG_TRACE("rx: {}", *req);

    switch(req -> req_type) {
        case _MEM_:
            ret = handle_mem_req(req, res, clifd);
            break;
        case _CPU_:
//            std::cout << "cpu req" << std::endl;
            ret = handle_cpu_usage_report(req, res);
            break;
        case _INIT_:
            ret = Manager::handle_add_cgroup_to_ec(req, res, host_ip, clifd);
            break;
        default:
#if(SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE)
                std::cout << "Get ec_get_num_subcontainers(): " << ec_get_num_subcontainers() << std::endl;
#endif
            SPDLOG_ERROR("Handling memory/cpu request failed! manager_id: {}", manager_id);
    }
    return ret;
}

int ec::Manager::handle_add_cgroup_to_ec(const ec::msg_t *req, ec::msg_t *res, uint32_t ip, int fd) {
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
    SPDLOG_TRACE("fair share: {}", ec_get_fair_cpu_share());

    // And so once a subcontainer is created and added to the appropriate distributed container,
    // we can now create a map to link the container_id and agent_client
    AgentClientDB* acdb = AgentClientDB::get_agent_client_db_instance();
    auto agent_ip = sc->get_c_id()->server_ip;
    auto target_agent = acdb->get_agent_client_by_ip(agent_ip);
    if ( target_agent ){
        std::lock_guard<std::mutex> lk(cv_mtx);
        _ec->add_to_sc_ac_map(*sc->get_c_id(), target_agent);
        SPDLOG_DEBUG("manager thread: map for update, ref sizes: {}, {}",
                     _ec->get_sc_ac_map_for_update()->size(), _ec->get_sc_ac_map().size());
        sc->set_sc_inserted(true);
        cv.notify_one();
    } else {
        SPDLOG_ERROR("SubContainer's node IP or Agent IP not found!");
    }

    SPDLOG_DEBUG("update_quota_flag: {}", update_quota);
    //Update pod quota
    if(update_quota) {
        set_sc_quota_syscall(sc, quota, 13);
    }

    //update pod mem limit
    std::thread update_mem_limit_thread(&ec::Manager::determine_mem_limit_for_new_pod, this, sc, fd);
    update_mem_limit_thread.detach();

    SPDLOG_INFO("total pods added to map: {}", ec_get_num_subcontainers());
    res->request += 1; //giveback (or send back)
    return ret;
}

void ec::Manager::determine_mem_limit_for_new_pod(ec::SubContainer *sc, int clifd) {
    if(!sc) {
        SPDLOG_ERROR("sc in determine_mem_limit_for_new_pod() is NULL");
        return;
    }

    int count = 0;
    SPDLOG_DEBUG("in determine_new_limit_for_new_pod. sc_id: {}", *sc->get_c_id());
    while(sc->get_docker_id().empty()) {
//        SPDLOG_DEBUG("waiting for docker_id to not be empty. sc_id: {}", *sc->get_c_id());
        count++;
        if(count % 1000 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            SPDLOG_DEBUG("count for waiting for dockid to not be empty: {}, sc_id: {}", count, *sc->get_c_id());
        }

    }
    SPDLOG_DEBUG("docker_id not empty! count: {}, sc_id: {}", count, *sc->get_c_id());

    auto mem_lim_bytes = sc_get_memory_limit_in_bytes_cadvisor(*sc->get_c_id());
    SPDLOG_DEBUG("post sc_get mem limit bytes cadvisor");
    auto sc_mem_limit_in_pages = byte_to_page(mem_lim_bytes);
    SPDLOG_INFO("mem_lim_bytes, pages: {}, {}", mem_lim_bytes, sc_mem_limit_in_pages);

    SPDLOG_DEBUG("ec_get_unalloc_mem rn: {}", ec_get_unalloc_memory_in_pages());
    SPDLOG_DEBUG("sc_mem_limit_in_pages on deploy: {}", sc_mem_limit_in_pages);

    if(sc_mem_limit_in_pages <= ec_get_unalloc_memory_in_pages()) {
        SPDLOG_DEBUG("mem lim in pages <= unalloc mem limit. let's update alloc mem");
        ec_update_alloc_memory_in_pages(sc_mem_limit_in_pages);
    }
    else if(!ec_get_unalloc_memory_in_pages()) {
        SPDLOG_ERROR("No Memory available to allocate to newly connected container!");
        SPDLOG_ERROR("tbh not sure how to handle this");
    }
    else if(sc_mem_limit_in_pages > ec_get_unalloc_memory_in_pages()) {
        SPDLOG_DEBUG("new sc mem limit > unalloc mem in pages. reclaim");
        //need to get back memory from other pods
        auto reclaimed_mem = handle_reclaim_memory(clifd);
        ec_update_reclaim_memory_in_pages(reclaimed_mem);
        SPDLOG_DEBUG("ec_get_unalloc_mem after reclaim: {}", ec_get_unalloc_memory_in_pages());
        if(sc_mem_limit_in_pages <= ec_get_unalloc_memory_in_pages()) {
            ec_update_alloc_memory_in_pages(sc_mem_limit_in_pages);
        }
        else {
            //mem_limit_in_pages still too high for mem_avail. try to reduce mem_limit_in_pages on pod
            SPDLOG_DEBUG("not enough pages reclaimed. trying to reduce mem limit on new pod");
            auto is_max_mem_resized = sc_resize_memory_limit_in_pages(*sc->get_c_id(), ec_get_unalloc_memory_in_pages());
            if(is_max_mem_resized) {
                SPDLOG_ERROR("Can't reclaim enough memory to deploy this new pod");
                SPDLOG_ERROR("tbh don't know how to handle this.");
            }
            else {
                ec_update_alloc_memory_in_pages(sc_mem_limit_in_pages);
            }
        }
    }
    SPDLOG_DEBUG("ec_get_unalloc_mem after mem alloc: {}", ec_get_unalloc_memory_in_pages());
    if(!sc) {
        SPDLOG_ERROR("ahhh sc is bad! in memory!");
    }
    sc->set_mem_limit_in_pages(sc_mem_limit_in_pages);
    SPDLOG_DEBUG("post set_mem_limit_in_pages() in memory on startup");
}


void ec::Manager::serveGrpcDeployExport() {
    std::string server_addr(deploy_service_ip + ":" + std::to_string(grpc_port));
    grpc::ServerBuilder builder;

    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(grpcServer);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    SPDLOG_DEBUG("Grpc Server listening on: {}", server_addr);
    server->Wait();
}

//TODO: this should be separated out into own file
[[noreturn]] void ec::Manager::run(const int num_containers) {

    while(ec_get_num_subcontainers() < num_containers) {
        sleep(5);
    }

    while (true)
    {

        uint64_t ret = 0;

        SPDLOG_TRACE("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
        SPDLOG_INFO("Periodic reclaim started for manager_id: {} from {} containers! ", manager_id, ec_get_num_subcontainers());

        for (const auto &sc_map : ec_get_subcontainers()) {
            ret += this->reclaim(sc_map.first, sc_map.second->back());
        }

        SPDLOG_INFO("Recalimed memory for manager_id: {} at the end of the periodic reclaim function (pages): {}", manager_id, ret);
        SPDLOG_TRACE("Recalimed memory at the end of the periodic reclaim function (MiB): {}", ret * 4096 / 1024 / 1024);
        if(ret > 0){
            ec_update_reclaim_memory_in_pages(ret); //no need to lock here. already locked in mem_h.cpp
        }
        else {
            SPDLOG_INFO("No memory reclaimed at end of reclamation process");
        }
        sleep(5);
        
    }
}




