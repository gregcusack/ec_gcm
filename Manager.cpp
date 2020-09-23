//
// Created by Greg Cusack on 11/5/19.
//

#include "Manager.h"

ec::Manager::Manager( int _manager_id, ec::ip4_addr gcm_ip, uint16_t server_port, std::vector<Agent *> &agents )
            : Server(_manager_id, gcm_ip, server_port, agents), ECAPI(_manager_id), manager_id(_manager_id),
            seq_number(0), cpuleak(0), deploy_service_ip(gcm_ip.to_string()), grpcServer(nullptr) {//, grpcServer(rpc::DeployerExportServiceImpl()) {

    //init server
    initialize();
}

void ec::Manager::start(const std::string &app_name,  const std::string &gcm_ip) {
    //A thread to listen for subcontainers' events
    std::thread event_handler_thread(&ec::Server::serve, this);
    ec::ECAPI::create_ec();
    grpcServer = new rpc::DeployerExportServiceImpl(_ec, cv, cv_dock, cv_mtx, cv_mtx_dock,sc_map_lock);
    std::thread grpc_handler_thread(&ec::Manager::serveGrpcDeployExport, this);
    sleep(10);

    std::cerr<<"[dbg] manager::just before running the app thread\n";
    std::thread application_thread(&ec::Manager::run, this);
    application_thread.join();
    grpc_handler_thread.join();
    event_handler_thread.join();

//    delete get
    delete getGrpcServer();
}

int ec::Manager::handle_cpu_usage_report(const ec::msg_t *req, ec::msg_t *res) {
    if (req == nullptr || res == nullptr) {
        SPDLOG_CRITICAL("req or res == null in handle_cpu_usage_report()");
        exit(EXIT_FAILURE);
    }
//    std::mutex cpulock;
    if (req->req_type != _CPU_) { return __ALLOC_FAILED__; }

    cpulock.lock();
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    if (!sc) {
        SPDLOG_ERROR("sc is NULL!");
        return __ALLOC_SUCCESS__;
    }
    sc->incr_counter();
    auto req_count = sc->get_counter();
    auto rx_quota = req->rsrc_amnt;
    auto rt_remaining = req->runtime_remaining;
    auto throttled = req->request;
    uint64_t updated_quota = rx_quota;
    uint64_t to_add = 0;
    int ret;
    uint64_t rx_buff;
    double thr_mean = 0;
    uint64_t rt_mean = 0;
    uint64_t total_rt_in_sys = 0;
    uint64_t tot_rt_and_overrun = 0;
    uint32_t seq_num = seq_number;

    if(rx_quota / 1000 != sc->get_quota() / 1000) {
        SPDLOG_ERROR("quotas do not match (rx, sc->get): ({}, {})", rx_quota, sc->get_quota());
        cpulock.unlock();
        res->request = 1;
        return __ALLOC_SUCCESS__;
    }

//    sc_map_lock.lock();
/*    for (const auto &i : get_subcontainers()) {
        //todo: need total_rt_in_sys as val like unalloc_rt -> update at every update.
        tot_rt_and_overrun += i.second->get_quota();
    }
    if(tot_rt_and_overrun != ec_get_alloc_rt()) {
        std::cout << "[MANAGER ERROR]: tot_rt != alloc_rt: (" << tot_rt_and_overrun << ", " << ec_get_alloc_rt() << ")" << std::endl;
    }
*/
//    std::cout << "tot_rt sum sc vs tot_alloc: (" << tot_rt_and_overrun << ", " << ec_get_alloc_rt() << ")" << std::endl;
//    std::cout << "rt in subcontainers: " << total_rt_in_sys << std::endl;
//    std::cout << "rt in unallocated pool: " << ec_get_cpu_unallocated_rt() << std::endl;
//    std::cout << "fair_share: " << ec_get_fair_cpu_share() << std::endl;
//    std::cout << "max cpu: " << ec_get_total_cpu() << std::endl;


//    sc_map_lock.unlock();

//    std::cout << "total rt given to containers: " << total_rt_in_sys << std::endl;
    total_rt_in_sys = ec_get_alloc_rt() + ec_get_cpu_unallocated_rt(); //alloc, overrun, unalloc
//    auto tot_rt_and_overrun = total_rt_in_sys + ec_get_overrun();
    //std::cout << "total rt in system, ovrn: " << total_rt_in_sys << ", " << ec_get_overrun() << std::endl;

    cpuleak += (int64_t)ec_get_total_cpu() - (int64_t)total_rt_in_sys;

    if( cpuleak >= _MAX_CPU_LOSS_IN_NS_) {
        SPDLOG_DEBUG("fix rt leak: tot cpuleak: {}", cpuleak);
        ec_incr_unallocated_rt(cpuleak);
        cpuleak = 0;
    }

//    std::cout << "total rt + overrun in system: " << tot_rt_and_overrun << std::endl;

    rt_mean = sc->get_cpu_stats()->insert_rt_stats(rt_remaining);
    thr_mean = sc->get_cpu_stats()->insert_th_stats(throttled);

    if(ec_get_overrun() > 0 && rx_quota > ec_get_fair_cpu_share()) {
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
        ret = set_sc_quota_syscall(sc, updated_quota, seq_num);
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
            ret = set_sc_quota_syscall(sc, updated_quota, seq_num);
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
            ret = set_sc_quota_syscall(sc, updated_quota, seq_num);
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
    else if(thr_mean >= 0.2 && ec_get_cpu_unallocated_rt() > 0) {  //sc_quota > fair share and container got throttled during the last period. need rt
        auto extra_rt = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(2 * thr_mean * ec_get_cpu_slice()));
        if(extra_rt > 0) {
            updated_quota = rx_quota + extra_rt;
            ret = set_sc_quota_syscall(sc, updated_quota, seq_num);
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
    else if(rt_mean > rx_quota * 0.2 && rx_quota >= ec_get_cpu_slice()) { //greater than 20% of quota unused
        uint64_t new_quota = rx_quota * (1 - 0.2); //sc_quota - sc_rt_remaining + ec_get_cpu_slice();
        new_quota = std::max(ec_get_cpu_slice(), new_quota);
        if(new_quota != rx_quota) {
            ret = set_sc_quota_syscall(sc, new_quota, seq_num); //give back what was used + 5ms
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
    else if(rt_mean > _MAX_UNUSED_RT_IN_NS_ && rx_quota >= ec_get_cpu_slice()) {
        uint64_t new_quota = rx_quota - _MAX_UNUSED_RT_IN_NS_;
        new_quota = std::max(ec_get_cpu_slice(), new_quota);
        if(new_quota != rx_quota) {
            ret = set_sc_quota_syscall(sc, new_quota, seq_num); //give back what was used + 5ms
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

    seq_number++;
    res->request = 1;
    cpulock.unlock();
    return __ALLOC_SUCCESS__;

}

int ec::Manager::handle_mem_req(const ec::msg_t *req, ec::msg_t *res, int clifd) {
    if(req == nullptr || res == nullptr) {
        SPDLOG_CRITICAL("req or res == null in handle_mem_req()");
        exit(EXIT_FAILURE);
    }
    SPDLOG_DEBUG("handle_mem_req()");
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
            SPDLOG_DEBUG("no memory available!");
            res->rsrc_amnt = 0;
            return __ALLOC_MEM_FAILED__;
        }

    }

    SPDLOG_DEBUG("Handle mem req: success. memory available: {}", memory_available);
    ret = memory_available > ec_get_memory_slice() ? ec_get_memory_slice() : memory_available;

    SPDLOG_DEBUG("mem amnt to ret: {}", ret);

    ec_update_alloc_memory_in_pages(ret);

    SPDLOG_TRACE("successfully decrease unallocated mem to: {}", ec_get_unalloc_memory_in_pages());
    SPDLOG_TRACE("successfully increased allocated mem to: {}", ec_get_alloc_memory_in_pages());


    res->rsrc_amnt = req->rsrc_amnt + ret;   //give back "ret" pages
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    sc->set_mem_limit_in_pages(res->rsrc_amnt);

    memlock.unlock();
    res->request = 0;       //give back
    return __ALLOC_SUCCESS__;
}

//todo: reclaim memory should already know what each container mem limit is
uint64_t ec::Manager::handle_reclaim_memory(int client_fd) {
    uint64_t total_reclaimed = 0;

    SPDLOG_DEBUG("GCM: Trying to reclaim memory from other cgroups!");
    for (const auto &container : ec_get_subcontainers()) {
        if (container.second->get_fd() == client_fd) {
            continue;
        }
        auto mem_limit_pages = container.second->get_mem_limit_in_pages();
        auto mem_limit_bytes = page_to_byte(mem_limit_pages);

        auto mem_usage_bytes = sc_get_memory_usage_in_bytes(container.first);
        if(mem_limit_bytes - mem_usage_bytes > _SAFE_MARGIN_BYTES_) {
            auto is_max_mem_resized = sc_resize_memory_limit_in_pages(container.first,
                                                                      byte_to_page(mem_usage_bytes + _SAFE_MARGIN_BYTES_));
            SPDLOG_TRACE("byte to page macro output: {}", byte_to_page(mem_limit_bytes - (mem_usage_bytes + _SAFE_MARGIN_BYTES_)));
            SPDLOG_TRACE("is_max_mem_resized: {}", is_max_mem_resized);
            if(!is_max_mem_resized) {
                total_reclaimed += byte_to_page(mem_limit_bytes - (mem_usage_bytes + _SAFE_MARGIN_BYTES_));
                sc_set_memory_limit_in_pages(*container.second->get_c_id(), byte_to_page(mem_usage_bytes + _SAFE_MARGIN_BYTES_));
            }
        }
        else {
            SPDLOG_DEBUG("mem usage to close to mem_limit_bytes to resize! --> limit - usage: {}", mem_limit_bytes - mem_usage_bytes);
            SPDLOG_DEBUG("safe margin: {}", _SAFE_MARGIN_BYTES_);
        }
    }
    SPDLOG_DEBUG("Recalimed memory at the end of the reclaim function: {}", total_reclaimed);
    return total_reclaimed;
}

int ec::Manager::handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd){
    if(!req || !res) {
        SPDLOG_CRITICAL("req or res == null in handle_req()"); //todo: we shouldn't exit_failure on one null req
        exit(EXIT_FAILURE);
    }
    uint64_t ret = __FAILED__;

    switch(req -> req_type) {
        case _MEM_:
            ret = handle_mem_req(req, res, clifd);
            break;
        case _CPU_:
            ret = handle_cpu_usage_report(req, res);
            break;
        case _INIT_:
            ret = Manager::handle_add_cgroup_to_ec(req, res, host_ip, clifd);
            break;
        default:
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
        cv.notify_one();
    } else {
        SPDLOG_ERROR("SubContainer's node IP or Agent IP not found!");
    }

    //Update pod quota
    if(update_quota) {
        std::thread update_quota_thread(&ec::Manager::set_sc_quota_syscall, this, sc, quota, 13);
        update_quota_thread.detach();
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

    std::unique_lock<std::mutex> lk_dock(cv_mtx_dock);
    cv_dock.wait(lk_dock, [this, sc] {
        return !sc->get_docker_id().empty();
    });
    auto sc_mem_limit_in_pages = byte_to_page(sc_get_memory_limit_in_bytes(*sc->get_c_id()));

    SPDLOG_TRACE("ec_get_unalloc_mem rn: {}", ec_get_unalloc_memory_in_pages());
    SPDLOG_TRACE("sc_mem_limit_in_pages on deploy: {}", sc_mem_limit_in_pages);

    if(sc_mem_limit_in_pages <= ec_get_unalloc_memory_in_pages()) {
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
    SPDLOG_TRACE("ec_get_unalloc_mem after mem alloc: {}", ec_get_unalloc_memory_in_pages());
    sc->set_mem_limit_in_pages(sc_mem_limit_in_pages);
}


void ec::Manager::serveGrpcDeployExport() {
    std::string server_addr(deploy_service_ip + ":4447");
    grpc::ServerBuilder builder;

    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(grpcServer);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    SPDLOG_DEBUG("Grpc Server listening on: {}", server_addr);
    server->Wait();
}

//TODO: this should be separated out into own file
void ec::Manager::run() {
    //ec::SubContainer::ContainerId x ;
//    std::cout << "[dbg] In Manager Run function" << std::endl;
//    std::cout << "EC Map Size: " << _ec->get_subcontainers().size() << std::endl;
    while(true){
//        for(auto sc_ : _ec->get_subcontainers()){
//            std::cout << "=================================================================================================" << std::endl;
//            std::cout << "[READ API]: the memory limit and max_usage in bytes of the container with cgroup id: " << sc_.second->get_c_id()->cgroup_id << std::endl;
//            std::cout << " on the node with ip address: " << sc_.first.server_ip  << " is: " << sc_get_memory_limit_in_bytes(sc_.first) << "---" << sc_get_memory_usage_in_bytes(sc_.first) << std::endl;
//            std::cout << "[READ API]: machine free: " << get_machine_free_memory(sc_.first) << std::endl;
//            std::cout << "=================================================================================================\n";
//            std::cout << "quota is: " << get_cpu_quota_in_us(sc_.first) << "###" << std::endl;
//            sleep(1);
        }
//        std::cout << "&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&" << std::endl;
//        sleep(10);
//    }
}





