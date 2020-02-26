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
//    std::cout << "------------------------------" << std::endl;
//    std::cout << "cg id: " << req->cgroup_id << std::endl;
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    if(!sc) {
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
    uint64_t total_rt = 0;

    for(const auto &i : get_subcontainers()) {
        total_rt += i.second->sc_get_quota();
    }
    total_rt += ec_get_cpu_unallocated_rt();
//    std::cout << "total rt: " << total_rt << std::endl;
//    if(ec_get_fair_cpu_share() * get_subcontainers().size() < total_rt) {
//        std::cout << "WOOPS! ALLOC TOO MUCH CPU!" << std::endl;
//    }

//    std::cout << "quota, rt_remaining, throttled): (" << rx_quota << ", " << rt_remaining << ", " << throttled << ")" << std::endl;
//    //TODO: when we change quota, we need to flush the window
//    if(likely(!sc->get_set_quota_flag())) {
//        rt_mean = sc->get_cpu_stats()->insert_rt_stats(rt_remaining);
//        thr_mean = sc->get_cpu_stats()->insert_th_stats(throttled);
//    } else {
//        rt_mean = sc->get_cpu_stats()->get_rt_mean();
//        thr_mean = sc->get_cpu_stats()->get_thr_mean();
//        sc->set_quota_flag(false);
//    }
    rt_mean = sc->get_cpu_stats()->insert_rt_stats(rt_remaining);
    thr_mean = sc->get_cpu_stats()->insert_th_stats(throttled);
//    std::cout << "rt_mean: " << rt_mean << std::endl;
//    std::cout << "thr_mean: " << thr_mean << std::endl;
//    std::cout << "cpu_unalloc: " << ec_get_cpu_unallocated_rt() << std::endl;

    if(ec_get_overrun() > 0 && rx_quota > ec_get_fair_cpu_share()) {
//        std::cout << "overrun. sc: " << *sc->get_c_id() << std::endl;
        uint64_t to_sub;
        uint64_t amnt_share_over = rx_quota - ec_get_fair_cpu_share();
        uint64_t overrun = ec_get_overrun();
        double percent_over = ((double)rx_quota - (double)ec_get_fair_cpu_share()) / (double)ec_get_fair_cpu_share();
//        std::cout << "percent over: " << percent_over << std::endl;
        if(percent_over > 1) {
            to_sub = (uint64_t) (percent_over * (double) ec_get_cpu_slice());
        }
        else {
            to_sub = (uint64_t) (percent_over * (double)amnt_share_over);
        }
        //TODO: thr_mean is probably important. Take less from containers that are constantly being throttled
//        uint64_t to_sub_frac = (1 - thr_mean) * amnt_share_over;
        if(to_sub < ec_get_cpu_slice() / 2) { //ensures we eventually converge
            to_sub = amnt_share_over;
        }
        to_sub = std::min(overrun, to_sub);
        updated_quota = rx_quota - to_sub;
        ret = set_sc_quota(sc, updated_quota);
        if(ret <= 0) {
            std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (overrun sub quota). ret: " << ret << std::endl;
        }
        else {
            sc->set_quota_flag(true);
            std::cout << "successfully resized quota to: " << updated_quota << "!" << std::endl;
        }
        sc->get_cpu_stats()->flush();
        ec_decr_overrun(to_sub);
        sc->sc_set_quota(updated_quota);
    }
    else if(rx_quota < ec_get_fair_cpu_share() && thr_mean > 0.5) {   //throttled but don't have fair share
//        std::cout << "throt and less than fair share. sc: " << *sc->get_c_id() << std::endl;
        uint64_t amnt_share_lacking = ec_get_fair_cpu_share() - rx_quota;
//        std::cout << "amnt_share_lacking: " << amnt_share_lacking << std::endl;
        if (ec_get_cpu_unallocated_rt() > 0) {
            //TODO: take min of to_Add and slice. don't full reset
//            std::cout << "give back some unalloc_Rt. sc: " << *sc->get_c_id() << std::endl;
            double percent_under = ((double)ec_get_fair_cpu_share() - (double)rx_quota) / (double)ec_get_fair_cpu_share();
            std::cout << "percent under1: " << percent_under << std::endl;
            if(amnt_share_lacking > ec_get_cpu_slice() / 2) {   //ensure we eventually converge
                to_add = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(percent_under * (double)amnt_share_lacking));
            }
            else {
                to_add = amnt_share_lacking;
            }
//            to_add = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(thr_mean * amnt_share_lacking));
//            std::cout << "to_Add: " << to_add << std::endl;
            updated_quota = rx_quota + to_add;
            ret = set_sc_quota(sc, updated_quota);
            if(ret <= 0) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr fair share). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                std::cout << "successfully resized quota to: " << updated_quota << "!" << std::endl;
            }
            ec_decr_unallocated_rt(to_add);
            sc->sc_set_quota(updated_quota);
//            std::cout << "new_quota: " << updated_quota << std::endl;
//            std::cout << "ec_get_unalloc_rt: " << ec_get_cpu_unallocated_rt() << std::endl;
        }
        else { //not enough in unalloc_rt to get back to fair share, even out
//            std::cout << "not enough in unalloc rt. give back slice or overrun.. sc: " << *sc->get_c_id() << std::endl;
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
            ret = set_sc_quota(sc, updated_quota);
            if(ret <= 0) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr fair share overrun). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                std::cout << "successfully resized quota to: " << updated_quota << "!" << std::endl;
            }
            ec_incr_overrun(overrun);
            sc->sc_set_quota(updated_quota);
        }
        sc->get_cpu_stats()->flush();
    }
    else if(thr_mean >= 0.2 && ec_get_cpu_unallocated_rt() > 0) {  //sc_quota > fair share and container got throttled during the last period. need rt
//        std::cout << "throttle. try get alloc. sc:  " << *sc->get_c_id() << std::endl;
        auto extra_rt = std::min(ec_get_cpu_unallocated_rt(), (uint64_t)(2 * thr_mean * ec_get_cpu_slice()));
        std::cout << "extra_rt: " << extra_rt << std::endl;
        if(extra_rt > 0) {
            ec_decr_unallocated_rt(extra_rt);
            updated_quota = rx_quota + extra_rt;
            ret = set_sc_quota(sc, updated_quota);
            if(ret <= 0) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                std::cout << "successfully resized quota to: " << rx_quota + extra_rt << "!" << std::endl;
            }
        }
//        else {
//            std::cout << "extra_rt == 0: " << extra_rt << std::endl;
//        }
        sc->sc_set_quota(updated_quota);
        sc->get_cpu_stats()->flush();
    }
    else if(rt_mean > rx_quota * 0.2) { //greater than 20% of quota unused
//        std::cout << "rt_mean > 20% of quota. sc: " << *sc->get_c_id() << std::endl;
        uint64_t new_quota = rx_quota * (1 - 0.2); //sc_quota - sc_rt_remaining + ec_get_cpu_slice();
        new_quota = std::max(ec_get_cpu_slice(), new_quota);
        if(new_quota != rx_quota) {
//            std::cout << "new, old, rt_remain: (" << new_quota << "," << rx_quota << "," << rt_mean << ")" << std::endl;
            ret = set_sc_quota(sc, new_quota); //give back what was used + 5ms
            if(ret <= 0) {
                std::cout << "[ERROR]: GCM. Can't read from socket to resize quota (incr). ret: " << ret << std::endl;
            }
            else {
                sc->set_quota_flag(true);
                std::cout << "successfully resized quota to: " << new_quota << "!" << std::endl;
            }
//            std::cout << "old quota, new quota: (" << rx_quota << ", " << new_quota << ")" << std::endl;
            ec_incr_unallocated_rt(rx_quota - new_quota); //unalloc_rt <-- old quota - new quota
            sc->sc_set_quota(new_quota);
            sc->get_cpu_stats()->flush();
        }
//        else {
//            std::cout << "new_quota == old_quota: " << new_quota << std::endl;
//        }
    }
//    else {
//        std::cout << "DO NOTHING" << std::endl;
//    }

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
//            ret = read(agentClient->get_socket(), buffer, sizeof(buffer));
//            return ret;
            delete reclaim_req;
            return 1;
        }
    }
    return -1;
}



