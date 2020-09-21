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
        std::cout << "req or res == null in handle_cpu_usage_report()" << std::endl;
        exit(EXIT_FAILURE);
    }
//    std::mutex cpulock;
    if (req->req_type != _CPU_) { return __ALLOC_FAILED__; }

    cpulock.lock();
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    if (!sc) {
        std::cout << "ERROR! sc is NULL!" << std::endl;
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
        std::cout << "quotas do not match (rx, sc->get): (" << rx_quota << ", " << sc->get_quota() << ")" << std::endl;
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
        std::cout << "fix rt leak: tot cpuleak: " << cpuleak << std::endl;
        ec_incr_unallocated_rt(cpuleak);
        cpuleak = 0;
    }

//    std::cout << "total rt + overrun in system: " << tot_rt_and_overrun << std::endl;

    rt_mean = sc->get_cpu_stats()->insert_rt_stats(rt_remaining);
    thr_mean = sc->get_cpu_stats()->insert_th_stats(throttled);

//    std::cout << "sc with id: " << *sc->get_c_id() << " ----  rt_mean: " << rt_mean << std::endl;
//    std::cout << "sc with id: " << *sc->get_c_id() << " ----  thr_mean: " << thr_mean << std::endl;

//    std::cout << "cpu_unalloc: " << ec_get_cpu_unallocated_rt() << std::endl;

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
            std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (overrun sub quota). ret: " << ret << std::endl;
        }
        else {
            sc->set_quota_flag(true);
            //std::cout << "successfully resized quota to (overrun): " << updated_quota << "!" << std::endl;
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
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr fair share). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                //std::cout << "successfully resized quota to (fair share 1): " << updated_quota << "!" << std::endl;
                ec_decr_unallocated_rt(to_add);
                sc->set_quota(updated_quota);
                sc->get_cpu_stats()->flush();
                ec_incr_alloc_rt(to_add);
            }
        }
        else { //not enough in unalloc_rt to get back to fair share, even out
            uint64_t overrun;
            double percent_under = ((double)ec_get_fair_cpu_share() - (double)rx_quota) / (double)ec_get_fair_cpu_share();
//            std::cout << "percent under2: " << percent_under << std::endl;
//            overrun = (uint64_t)thr_mean * amnt_share_lacking;
            if(amnt_share_lacking > ec_get_cpu_slice() / 2) { //ensure we eventualyl converge
                overrun = (uint64_t)((double) percent_under * amnt_share_lacking);
            }
            else {
                overrun = amnt_share_lacking;
            }
            updated_quota = rx_quota + overrun;
            ret = set_sc_quota_syscall(sc, updated_quota, seq_num);
            if(ret) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr fair share overrun). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                //std::cout << "successfully resized quota to (fair share 2): " << updated_quota << "!" << std::endl;
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
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                //std::cout << "successfully resized quota to (incr): " << rx_quota + extra_rt << "!" << std::endl;
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
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (decr). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                //std::cout << "successfully resized quota to (decr): " << new_quota << "!" << std::endl;
                //std::cout << "decr resize (rx_q, new_q): (" << rx_quota << ", " << new_quota << ")" << std::endl;
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
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (decr 5ms diff). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                //std::cout << "successfully resized quota to (decr 5ms diff): " << new_quota << "!" << std::endl;
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
        std::cout << "req or res == null in handle_mem_req()" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "handle_mem_req()" << std::endl;
    uint64_t ret = 0;
    if(req->req_type != _MEM_) { res->rsrc_amnt = 0; return __ALLOC_MEM_FAILED__; }
    memlock.lock();
    int64_t memory_available = ec_get_unalloc_memory_in_pages();
    if(memory_available > 0) {
        std::cout << "gcm memory avail > 0. Returning global slice. mem avail: " << memory_available << std::endl;
    }
    else {
        auto reclaimed_mem = handle_reclaim_memory(clifd);
        if(reclaimed_mem > 0) {
            if(memory_available < 0) {
                std::cout << "ERROR: reclaiming memory and mem_available < 0. THis should not happen!!" << std::endl;
                std::cout << "----> mem_avail: " << memory_available << ", reclaimed_mem: " << reclaimed_mem << "\n";
                std::cout << "----> alloc_mem: " << ec_get_alloc_memory_in_pages() << ", appl_mem_limit: " <<
                          ec_get_mem_limit_in_pages() << std::endl;
                //SETTING MEM_AVAIL TO 0 AND GOING FROM THERE
                ec_incr_unalloc_memory_in_pages(memory_available * -1);
                ec_decr_alloc_memory_in_pages(memory_available * -1);
            }
            ec_update_reclaim_memory_in_pages(reclaimed_mem);
            memory_available = ec_get_unalloc_memory_in_pages();
            std::cout << "reclaimed mem from other pods. mem rx: " << memory_available << std::endl;
        }
        else {
            memlock.unlock();
            std::cout << "no memory available!" << std::endl;
            res->rsrc_amnt = 0;
            return __ALLOC_MEM_FAILED__;
        }

    }

    std::cout << "Handle mem req: success. memory available: " << memory_available << std::endl;
    ret = memory_available > ec_get_memory_slice() ? ec_get_memory_slice() : memory_available;

    std::cout << "mem amnt to ret: " << ret << std::endl;

    ec_update_alloc_memory_in_pages(ret);

    std::cout << "successfully decrease unallocated mem to: " << ec_get_unalloc_memory_in_pages() << std::endl;
    std::cout << "successfully increased allocated mem to: " << ec_get_alloc_memory_in_pages() << std::endl;


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

    std::cout << "[INFO] GCM: Trying to reclaim memory from other cgroups!" << std::endl;
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
            std::cout << "[dbg] byte to page macro output: " << byte_to_page(mem_limit_bytes - (mem_usage_bytes + _SAFE_MARGIN_BYTES_)) << std :: endl;
            std::cout << "[dbg] is_max_mem_resized: " << is_max_mem_resized << std::endl;
            if(!is_max_mem_resized) {
                total_reclaimed += byte_to_page(mem_limit_bytes - (mem_usage_bytes + _SAFE_MARGIN_BYTES_));
                sc_set_memory_limit_in_pages(*container.second->get_c_id(), byte_to_page(mem_usage_bytes + _SAFE_MARGIN_BYTES_));
            }
        }
        else {
            std::cout << "mem usage to close to mem_limit_bytes to resize! --> limit - usage: " << mem_limit_bytes - mem_usage_bytes << std::endl;
            std::cout << "safe margin: " << _SAFE_MARGIN_BYTES_ << std::endl;
        }
    }
    std::cout << "[dbg] Recalimed memory at the end of the reclaim function: " << total_reclaimed << std::endl;
    return total_reclaimed;
}

int ec::Manager::handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd){
    if(!req || !res) {
        std::cout << "req or res == null in handle_req()" << std::endl;
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
            std::cout << "[Error]: ECAPI: " << manager_id << ". Handling memory/cpu request failed!" << std::endl;
    }
    return ret;
}

int ec::Manager::handle_add_cgroup_to_ec(const ec::msg_t *req, ec::msg_t *res, uint32_t ip, int fd) {
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

    //std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_c_id() << std::endl;
    AgentClientDB* acdb = AgentClientDB::get_agent_client_db_instance();
    auto agent_ip = sc->get_c_id()->server_ip;
    auto target_agent = acdb->get_agent_client_by_ip(agent_ip);
    //std::cout << "[dbg] Agent client ip: " << target_agent-> get_agent_ip() << std::endl;
    //std::cout << "[dbg] Agent ip: " << agent_ip << std::endl;
    if ( target_agent ){
//        mtx.lock();
        //std::cout << "add to sc_ac map" << std::endl;
        std::lock_guard<std::mutex> lk(cv_mtx);
        _ec->add_to_sc_ac_map(*sc->get_c_id(), target_agent);
        //std::cout << "handle() sc_id, agent_ip: " << *sc->get_c_id() << ", " << target_agent->get_agent_ip() << std::endl;
        cv.notify_one();
//        mtx.unlock();
    } else {
        std::cerr<< "[ERROR] SubContainer's node IP or Agent IP not found!" << std::endl;
    }

    //Update pod quota
    if(update_quota) {
        std::thread update_quota_thread(&ec::Manager::set_sc_quota_syscall, this, sc, quota, 13);
        update_quota_thread.detach();
    }

    //update pod mem limit
    std::thread update_mem_limit_thread(&ec::Manager::determine_mem_limit_for_new_pod, this, sc, fd);
    update_mem_limit_thread.detach();

    //std::cout << "returning from handle_Add_cgroup_to_ec(): ret: " << ret << std::endl;
    std::cout << "total pods added to map: " << ec_get_num_subcontainers() << std::endl;
    res->request += 1; //giveback (or send back)
    return ret;
}

void ec::Manager::determine_mem_limit_for_new_pod(ec::SubContainer *sc, int clifd) {
    if(!sc) {
        std::cerr << "sc in determine_mem_limit_for_new_pod() is NULL" << std::endl;
        return;
    }
//    uint64_t sc_mem_limit_in_pages = 0;
//    std::cout << "determining mem limit for new pod: " << sc << std::endl;
    std::unique_lock<std::mutex> lk_dock(cv_mtx_dock);
    cv_dock.wait(lk_dock, [this, sc] {
        //std::cout << "in wait for docker id to be set" << std::endl;
        return !sc->get_docker_id().empty();
    });
    auto sc_mem_limit_in_pages = byte_to_page(sc_get_memory_limit_in_bytes(*sc->get_c_id()));
//    std::cout << "ec_get_unalloc_mem rn: " << ec_get_unalloc_memory_in_pages() << std::endl;
//    std::cout << "sc_mem_limit_in_pages on deploy: " << sc_mem_limit_in_pages << std::endl;
    if(sc_mem_limit_in_pages <= ec_get_unalloc_memory_in_pages()) {
        ec_update_alloc_memory_in_pages(sc_mem_limit_in_pages);
    }
    else if(!ec_get_unalloc_memory_in_pages()) {
        std::cerr << "[ERROR]: No Memory available to allocate to newly connected container!" << std::endl;
        std::cerr << "tbh not sure how to handle this" << std::endl;
    }
    else if(sc_mem_limit_in_pages > ec_get_unalloc_memory_in_pages()) {
        std::cout << "new sc mem limit > unalloc mem in pages. reclaim" << std::endl;
        //need to get back memory from other pods
        auto reclaimed_mem = handle_reclaim_memory(clifd);
        ec_update_reclaim_memory_in_pages(reclaimed_mem);
        std::cout << "ec_get_unalloc_mem after reclaim: " << ec_get_unalloc_memory_in_pages() << std::endl;
        if(sc_mem_limit_in_pages <= ec_get_unalloc_memory_in_pages()) {
            ec_update_alloc_memory_in_pages(sc_mem_limit_in_pages);
        }
        else {
            //mem_limit_in_pages still too high for mem_avail. try to reduce mem_limit_in_pages on pod
            std::cout << "not enough pages reclaimed. trying to reduce mem limit on new pod" << std::endl;
            auto is_max_mem_resized = sc_resize_memory_limit_in_pages(*sc->get_c_id(), ec_get_unalloc_memory_in_pages());
            if(is_max_mem_resized) {
                std::cerr << "[ERROR]: Can't reclaim enough memory to deploy this new pod" << std::endl;
                std::cout << "tbh don't know how to handle this." << std::endl;
            }
            else {
                ec_update_alloc_memory_in_pages(sc_mem_limit_in_pages);
            }
        }
    }
//    std::cout << "ec_get_unalloc_mem after mem alloc: " << ec_get_unalloc_memory_in_pages() << std::endl;
    sc->set_mem_limit_in_pages(sc_mem_limit_in_pages);
}


void ec::Manager::serveGrpcDeployExport() {
    std::string server_addr(deploy_service_ip + ":4447");
    grpc::ServerBuilder builder;

    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(grpcServer);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Grpc Server listening on: " << server_addr << std::endl;
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





