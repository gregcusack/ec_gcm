//
// Created by Greg Cusack on 11/5/19.
//

#include "Manager.h"

#include <chrono>

int ec::Manager::handle_cpu_usage_report(const ec::msg_t *req, ec::msg_t *res) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_cpu_usage_report()" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::mutex cpulock;
    if(req->req_type != _CPU_) { return __ALLOC_FAILED__; }

    auto t1 = std::chrono::high_resolution_clock::now();
    cpulock.lock();
    std::cout << "cg id: " << req->cgroup_id << std::endl;
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    sc->incr_counter();
    auto req_count = sc->get_counter();
    auto quota = req->rsrc_amnt;
    auto rt_remaining = req->runtime_remaining;
    auto throttled = req->request;
    int ret;
    char buffer[__BUFF_SIZE__] = {0};
    uint64_t rx_buff;
    double thr_mean = 0;
    uint64_t rt_mean = 0;

    std::cout << "rx: (quota, rt_remaining, throttled): (" << quota << ", " << rt_remaining << ", " << throttled << ")" << std::endl;
    //TODO: when we change quota, we need to flush the window
    if(likely(!sc->get_set_quota_flag())) {
        rt_mean = sc->get_cpu_stats()->insert_rt_stats(rt_remaining);
        thr_mean = sc->get_cpu_stats()->insert_th_stats(throttled);
    } else {
        rt_mean = sc->get_cpu_stats()->get_rt_mean();
        thr_mean = sc->get_cpu_stats()->get_thr_mean();
        sc->set_quota_flag(false);
    }

    std::cout << "rt_mean: " << rt_mean << std::endl;
    std::cout << "thr_mean: " << thr_mean << std::endl;

//
//    if(throttle_incr > 0 && ec_get_cpu_unallocated_rt() > 0) {  //sc_quota > fair share and container got throttled during the last period. need rt
//        std::cout << "throttle. try get alloc. sc:  " << *sc->get_c_id() << std::endl;
//        auto extra_rt = std::min(ec_get_cpu_unallocated_rt(), ec_get_cpu_slice());
//        ec_decr_unallocated_rt(extra_rt);
//        sc->sc_set_quota(sc_quota + extra_rt);
//    }

    if(req_count % 100 == 0 && req_count % 200 != 0) {
        ret = set_sc_quota(sc, 20000000);
        if(ret <= 0) {
            std::cout << "[ERROR]: GCM. Can't read from socket to resize quota. ret: " << ret << std::endl;
        }
        else {
            sc->set_quota_flag(true);
            std::cout << "successfully resized quota to: " << 20000000 << "!" << std::endl;
        }
    }
    else if(req_count % 200 == 0) {
        ret = set_sc_quota(sc, 50000000);
        if(ret <= 0) {
            std::cout << "[ERROR]: GCM. Can't read from socket to resize quota. ret: " << ret << std::endl;
        }
        else {
            sc->set_quota_flag(true);
            std::cout << "successfully resized quota to: " << 50000000 << "!" << std::endl;
        }
    }




//    auto sc = ec_get_sc_for_update(sc_id);
//    auto sc_quota = sc->sc_get_quota();
//    auto sc_quota_rx = req->rsrc_amnt;
//    if(sc_quota != sc_quota_rx) {
//        std::cout << "QUOTA MISMATCH (rx, in_gcm): (" << sc_quota_rx << ", " << sc_quota << ")" << std::endl;
//    }
//
////    auto sc_quota = req->rsrc_amnt;
////    std::cout << "rx quota: " << sc_quota << std::endl;
//    auto sc_rt_remaining = req->runtime_remaining;
//    uint32_t throttle_incr = sc->sc_get_thr_incr_and_set_thr(req->request);
//    std::cout << "get overrun. " << ec_get_overrun() << std::endl;
//
//    /* TODO: uncomment this whole block
//     * container has more than fair cpu share and gcm has overallocated rt. reduce rt.
//     */
////    if(ec_get_overrun() > 0 && sc_quota > ec_get_fair_cpu_share()) {
////        std::cout << "overrun" << std::endl;
////        uint64_t amnt_share_over = sc_quota - ec_get_fair_cpu_share();
////        sc->sc_set_quota(ec_get_fair_cpu_share());
////        ec_decr_overrun(amnt_share_over);
////    }
//
//    if(ec_get_overrun() > 0 && sc_quota > ec_get_fair_cpu_share()) {
//        std::cout << "overrun. sc: " << *sc->get_c_id() << std::endl;
//        uint64_t amnt_share_over = sc_quota - ec_get_fair_cpu_share();
//        uint64_t new_quota, to_sub, overrun_sub;
//        uint64_t overrun = ec_get_overrun();
//        //subtract back to fair share or remove slice if overrun > slice and amnt_fair_shair > overrun
//        /*
//         * Get back to fair share OR reduce quota by slice. depends on overrun size.
//         * don't want to subtract more than we're overrun
//         */
//        if(amnt_share_over >= ec_get_cpu_slice()) {
//            to_sub = std::min(overrun, ec_get_cpu_slice()); //
//        }
//        else {
//            to_sub = std::min(overrun, amnt_share_over);
//        }
//        new_quota = sc_quota - to_sub;
//        overrun_sub = to_sub;
//
//        sc->sc_set_quota(new_quota);
//        ec_decr_overrun(overrun_sub);
//    }
//    /* TODO: uncomment this whole block
//     * Give back rt to container that is throttle and has less than it's fair share
//     */
//    else if(throttle_incr > 0 && sc_quota < ec_get_fair_cpu_share()) {   //throttled but don't have fair share
//        std::cout << "throt and less than fair share. sc: " << *sc->get_c_id() << std::endl;
//        uint64_t amnt_share_lacking = ec_get_fair_cpu_share() - sc_quota;
//        std::cout << "amnt_share_lacking: " << amnt_share_lacking << std::endl;
//        if(ec_get_cpu_unallocated_rt() > 0) {
//            //TODO: take min of to_Add and slice. don't full reset
//            std::cout << "give back some unalloc_Rt. sc: " << *sc->get_c_id() << std::endl;
//            uint64_t to_add = std::min(ec_get_cpu_unallocated_rt(), amnt_share_lacking);
//            std::cout << "to_Add: " << to_add << std::endl;
//            sc->sc_set_quota(sc_quota + to_add);
////            sc->sc_set_quota(to_add);
//            ec_decr_unallocated_rt(to_add);
//            sc_quota = sc->sc_get_quota();
//            std::cout << "new_quota: " << sc_quota << std::endl;
//            std::cout << "ec_get_unalloc_rt: " << ec_get_cpu_unallocated_rt() << std::endl;
//        }
//        if(sc_quota < ec_get_fair_cpu_share()) { //not enough in unalloc_rt to get back to fair share, even out
//            std::cout << "not enough in unalloc rt. give back slice or overrun.. sc: " << *sc->get_c_id() << std::endl;
//            amnt_share_lacking = ec_get_fair_cpu_share() - sc_quota;
//            uint64_t overrun, new_quota;
//            //check if we can add slice and not go over fair share
//            if(amnt_share_lacking > ec_get_cpu_slice()) {
//                new_quota = sc_quota + ec_get_cpu_slice();
//                overrun = ec_get_cpu_slice();
//            }
//            else { //if we add a slice, we will exceed fair share
//                new_quota = sc_quota + amnt_share_lacking;
//                overrun = amnt_share_lacking;
//            }
//            sc->sc_set_quota(new_quota);
//            ec_incr_overrun(overrun);
//        }
////        if(sc_quota < ec_get_fair_cpu_share()) { //not enough in unalloc_rt to get back to fair share, even out
////            std::cout << "reset to fair share" << std::endl;
////            uint64_t extra_rt = ec_get_fair_cpu_share() - sc_quota;
////            sc->sc_set_quota(ec_get_fair_cpu_share()); //just set to fair share
////            ec_incr_overrun(extra_rt);
////        }
//    }
//    //TODO: change back to else if
//    /*
//     * Container gets throttle and there is available rt. Give slice to container
//     */
//    else if(throttle_incr > 0 && ec_get_cpu_unallocated_rt() > 0) {  //sc_quota > fair share and container got throttled during the last period. need rt
//        std::cout << "throttle. try get alloc. sc:  " << *sc->get_c_id() << std::endl;
//        auto extra_rt = std::min(ec_get_cpu_unallocated_rt(), ec_get_cpu_slice());
//        ec_decr_unallocated_rt(extra_rt);
//        sc->sc_set_quota(sc_quota + extra_rt);
//    }
//    /*
//     * container does not use all of its runtime. give extra rt to unallocated_rt pool
//     */
//    else if(sc_rt_remaining > ec_get_cpu_slice()) { //greater than 5ms unused rt?
//        std::cout << "extra rt > slice size. sc: " << *sc->get_c_id() << std::endl;
//        uint64_t new_quota = sc_quota - sc_rt_remaining + ec_get_cpu_slice();
//
//        std::cout << "new, old, rt_remain, slice: (" << new_quota << "," << sc_quota << "," << sc_rt_remaining << "," << ec_get_cpu_slice() << ")" << std::endl;
//        sc->sc_set_quota(new_quota); //give back what was used + 5ms
//        std::cout << "old quota, new quota: (" << sc_quota << ", " << sc->sc_get_quota() << ")" << std::endl;
//        ec_incr_unallocated_rt(sc_quota - sc->sc_get_quota()); //unalloc_rt <-- old quota - new quota
//    }
//    else {
//        std::cout << "No need to change quota for ec: " << *sc->get_c_id() << std::endl;
//        std::cout << "quota, nr_throttle_dif, rt_remaining: (" << sc_quota << ", " << throttle_incr << ", " << sc_rt_remaining << std::endl;
//    }
//    std::cout << "unalloc rt: " << ec_get_cpu_unallocated_rt() << ". sc: " << *sc->get_c_id() << std::endl;
//    std::cout << "returning quota: " << sc->sc_get_quota() << ". sc: " << *sc->get_c_id() << std::endl;
//    res->rsrc_amnt = sc->sc_get_quota();
//    res->request = 0;
//    cpulock.unlock();
//    auto t2 = std::chrono::high_resolution_clock::now();
//    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
//    std::cout << "time to alloc: " << time_span.count() << "\n------------" << std::endl;

    return __ALLOC_SUCCESS__;

}

int ec::Manager::handle_mem_req(const ec::msg_t *req, ec::msg_t *res, int clifd) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_mem_req()" << std::endl;
        exit(EXIT_FAILURE);
    }
    uint64_t ret = 0;
    std::mutex memlock;
    if(req->req_type != _MEM_) { return __ALLOC_FAILED__; }
    memlock.lock();
    int64_t memory_available = ec_get_memory_available();
    if(memory_available > 0 || (memory_available = ec_set_memory_available(handle_reclaim_memory(clifd))) > 0) {          //TODO: integrate give back here
        std::cout << "Handle mem req: success. memory available: " << memory_available << std::endl;
        ret = memory_available > ec_get_memory_slice() ? ec_get_memory_slice() : memory_available;

        std::cout << "mem amnt to ret: " << ret << std::endl;

        ec_decrement_memory_available(ret);
//        memory_available -= ret;

        std::cout << "successfully decrease remaining mem to: " << memory_available << std::endl;

        res->rsrc_amnt = req->rsrc_amnt + ret;   //give back "ret" pages
        memlock.unlock();
        res->request = 0;       //give back
        return __ALLOC_SUCCESS__;
    }
    else {
        memlock.unlock();
        std::cout << "no memory available!" << std::endl;
        res->rsrc_amnt = 0;
        return __ALLOC_FAILED__;
    }
}

uint64_t ec::Manager::handle_reclaim_memory(int client_fd) {
    int j = 0;
    char buffer[__BUFF_SIZE__] = {0};
    uint64_t reclaimed = 0;
    uint64_t rx_buff;
    int ret;

    std::cout << "[INFO] GCM: Trying to reclaim memory from other cgroups!" << std::endl;
    for (const auto &container : get_subcontainers()) {
        if (container.second->get_fd() == client_fd) {
            continue;
        }
        auto ip = container.second->get_c_id()->server_ip;
        std::cout << "ac.size(): " << get_agent_clients().size() << std::endl;
        for (const auto &agentClient : get_agent_clients()) {
            std::cout << "(agentClient->ip, container ip): (" << agentClient->get_agent_ip() << ", " << ip << ")" << std::endl;
            if (agentClient->get_agent_ip() == ip) {
                auto *reclaim_req = new reclaim_msg;
                reclaim_req->cgroup_id = container.second->get_c_id()->cgroup_id;
                reclaim_req->is_mem = 1;
                //TODO: anyway to get the server to do this?
                if (write(agentClient->get_socket(), (char *) reclaim_req, sizeof(*reclaim_req)) < 0) {
                    std::cout << "[ERROR]: GCM EC Manager id: " << get_manager_id() << ". Failed writing to agent_clients socket"
                              << std::endl;
                }
                ret = read(agentClient->get_socket(), buffer, sizeof(buffer));
                if (ret <= 0) {
                    std::cout << "[ERROR]: GCM. Can't read from socke to reclaim memory" << std::endl;
                }
                rx_buff = *((uint64_t *) buffer);
                reclaimed += rx_buff;
                std::cout << "[INFO] GCM: reclaimed: " << rx_buff << " bytes" << std::endl;
                std::cout << "[INFO] GCM: Current amount of reclaimed memory: " << reclaimed << std::endl;
            }

        }

    }
    std::cout << "[dbg] Recalimed memory at the end of the reclaim function: " << reclaimed << std::endl;
    return reclaimed;
}

int ec::Manager::handle_add_cgroup_to_ec(const ec::msg_t *req, ec::msg_t *res, const uint32_t ip, int fd) {
    if(!res) {
        std::cout << "ERROR. res == null in handle_add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }
    auto *sc = _ec->create_new_sc(req->cgroup_id, ip, fd, req->rsrc_amnt, req->request);
    int ret = _ec->insert_sc(*sc);
    _ec->incr_total_cpu(sc->sc_get_quota());
    _ec->update_fair_cpu_share();
    std::cout << "fair share: " << ec_get_fair_cpu_share() << std::endl;
    std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_c_id() << std::endl;
    res->request = 0; //giveback (or send back)
    return ret;
}

int ec::Manager::set_sc_quota(ec::SubContainer *sc, uint64_t _quota) {
    if(!sc) {
        std::cout << "sc == NULL in manager set_sc_quota()" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    int ret = 0;
    char buffer[__BUFF_SIZE__] = {0};
    auto ip = sc->get_c_id()->server_ip;
    std::cout << "# clients (%100): " << get_agent_clients().size() << std::endl;
    for(const auto &agentClient : get_agent_clients()) {
        if(agentClient->get_agent_ip() == ip) {
            auto *reclaim_req = new reclaim_msg;
            reclaim_req->cgroup_id = sc->get_c_id()->cgroup_id;
            reclaim_req->is_mem = 0;
            reclaim_req->_quota = _quota;
            std::cout << "reclaim msg size: " << sizeof(*reclaim_req) << std::endl;
            if(write(agentClient->get_socket(), (char *) reclaim_req, sizeof(*reclaim_req)) < 0) {
                std::cout << "[ERROR]: GCM EC Manager id: " << get_manager_id() << ". Failed writing to agent_clients socket (resize)"
                          << std::endl;
            }
            ret = read(agentClient->get_socket(), buffer, sizeof(buffer));
            return ret;
        }
    }
    return -1;
}



