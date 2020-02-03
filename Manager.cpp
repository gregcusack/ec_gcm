//
// Created by Greg Cusack on 11/5/19.
//

#include "Manager.h"

int ec::Manager::handle_cpu_req(const ec::msg_t *req, ec::msg_t *res) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_cpu_req()" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::mutex cpulock;
    if(req->req_type != _CPU_) { return __ALLOC_FAILED__; }

    cpulock.lock();
    auto sc_id = SubContainer::ContainerId(req->cgroup_id, req->client_ip);
    auto sc = ec_get_sc_for_update(sc_id);
    auto sc_quota = sc->sc_get_quota();
    auto sc_quota_rx = req->rsrc_amnt;

    auto sc_rt_remaining = req->runtime_remaining;
    uint32_t throttle_incr = sc->sc_get_thr_incr_and_set_thr(req->request);

//    if(ec_get_overrun() > 0 && sc_quota > ec_get_fair_cpu_share()) {
//        std::cout << "overrun" << std::endl;
//        uint64_t amnt_share_over = sc_quota - ec_get_fair_cpu_share();
//        sc->sc_set_quota(ec_get_fair_cpu_share());
//        ec_decr_overrun(amnt_share_over);
//    }
    if(ec_get_overrun() > 0 && sc_quota > ec_get_fair_cpu_share()) {
        std::cout << "overrun" << std::endl;
        uint64_t amnt_share_over = sc_quota - ec_get_fair_cpu_share();
        uint64_t new_quota, to_sub, overrun_sub;
        uint64_t overrun = ec_get_overrun();
        //subtract back to fair share or remove slice if overrun > slice and amnt_fair_shair > overrun
        /*
         * Get back to fair share OR reduce quota by slice. depends on overrun size.
         * don't want to subtract more than we're overrun
         */
        if(amnt_share_over >= ec_get_cpu_slice()) {
            to_sub = std::min(overrun, ec_get_cpu_slice()); //
        }
        else {
            to_sub = std::min(overrun, amnt_share_over);
        }
        new_quota = sc_quota - to_sub;
        overrun_sub = to_sub;

        sc->sc_set_quota(new_quota);
        ec_decr_overrun(overrun_sub);
    }
    else if(throttle_incr > 0 && sc_quota < ec_get_fair_cpu_share()) {   //throttled but don't have fair share
        std::cout << "throt and less than fair share" << std::endl;
        uint64_t amnt_share_lacking = ec_get_fair_cpu_share() - sc_quota;
        if(ec_get_cpu_unallocated_rt() > 0) {
            std::cout << "give back some unalloc_Rt" << std::endl;
            uint64_t to_add = std::min(ec_get_cpu_unallocated_rt(), amnt_share_lacking);
            sc->sc_set_quota(to_add);
            ec_decr_unallocated_rt(to_add);
            sc_quota = sc->sc_get_quota();
        }
        if(sc_quota < ec_get_fair_cpu_share()) { //not enough in unalloc_rt to get back to fair share, even out
            amnt_share_lacking = ec_get_fair_cpu_share() - sc_quota;
            uint64_t overrun, new_quota;
            //check if we can add slice and not go over fair share
            if(amnt_share_lacking > ec_get_cpu_slice()) {
                new_quota = sc_quota + ec_get_cpu_slice();
                overrun = ec_get_cpu_slice();
            }
            else { //if we add a slice, we will exceed fair share
                new_quota = sc_quota + amnt_share_lacking;
                overrun = amnt_share_lacking;
            }
            sc->sc_set_quota(new_quota);
            ec_incr_overrun(overrun);
        }
//        if(sc_quota < ec_get_fair_cpu_share()) { //not enough in unalloc_rt to get back to fair share, even out
//            std::cout << "reset to fair share" << std::endl;
//            uint64_t extra_rt = ec_get_fair_cpu_share() - sc_quota;
//            sc->sc_set_quota(ec_get_fair_cpu_share()); //just set to fair share
//            ec_incr_overrun(extra_rt);
//        }
    }
    else if(throttle_incr > 0 && ec_get_cpu_unallocated_rt() > 0) {  //sc_quota > fair share and container got throttled during the last period. need rt
        std::cout << "throttle. try give alloc" << std::endl;
        auto extra_rt = std::min(ec_get_cpu_unallocated_rt(), ec_get_cpu_slice());
        ec_decr_unallocated_rt(extra_rt);
        sc->sc_set_quota(sc_quota + extra_rt);
    }
    else if(sc_rt_remaining > ec_get_cpu_slice()) { //greater than 5ms unused rt?
        std::cout << "extra rt > slice size" << std::endl;
        uint64_t new_quota = sc_quota - sc_rt_remaining + ec_get_cpu_slice();

        std::cout << "new, old, rt_remain, slice: (" << new_quota << "," << sc_quota << "," << sc_rt_remaining << "," << ec_get_cpu_slice() << ")" << std::endl;
        sc->sc_set_quota(new_quota); //give back what was used + 5ms
        std::cout << "old quota, new quota: (" << sc_quota << ", " << sc->sc_get_quota() << ")" << std::endl;
        ec_incr_unallocated_rt(sc_quota - sc->sc_get_quota()); //unalloc_rt <-- old quota - new quota
    }
    std::cout << "unalloc rt: " << ec_get_cpu_unallocated_rt() << std::endl;
    std::cout << "returning quota: " << sc->sc_get_quota() << std::endl;
    res->rsrc_amnt = sc->sc_get_quota();
    res->request = 0;
    cpulock.unlock();
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


