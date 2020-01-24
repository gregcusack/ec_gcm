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

    auto sc_rt_remaining = req->runtime_remaining;
    uint32_t throttle_incr = sc->sc_get_thr_incr_and_set_thr(req->request);
    if(throttle_incr > 0) {                                 //container got throttled during the last period. need rt
        std::cout << "throttle. try give alloc" << std::endl;
        auto extra_rt = std::min(ec_get_cpu_unallocated_rt(), ec_get_cpu_slice());
        if(extra_rt > 0) {
            std::cout << "need extra bw and some avail. giving back: " << extra_rt << std::endl;
            ec_decr_unallocated_rt(extra_rt);
            sc->sc_set_quota(sc_quota + extra_rt);
        }
    }
    else if(sc_rt_remaining > 0.2 * sc_quota) {             // is extra rt > than 20ms (if quota = 100ms)
        ec_incr_unallocated_rt(ec_get_cpu_slice());   // give cpu slice to global pool
        std::cout << "has extra rt. giving back slice to unalloc. unalloc_rt now: " << ec_get_cpu_unallocated_rt() << std::endl;
        sc->sc_set_quota(sc_quota - ec_get_cpu_slice());
    }

    std::cout << "returning quota: " << sc->sc_get_quota() << std::endl;
    res->rsrc_amnt = sc->sc_get_quota();
    res->request = 0;
    cpulock.unlock();
    return __ALLOC_SUCCESS__;
/*
    uint64_t ec_rt_remaining = ec_get_cpu_runtime_remaining();

    if(ec_rt_remaining > 0) {
        //give back what it asks for
        ret = req->rsrc_amnt > ec_rt_remaining ? ec_rt_remaining : req->rsrc_amnt;

//        runtime_remaining -= ret; //TODO: put back in at some point??
//        std::cout << "Server sending back " << ret << "ns in runtime" << std::endl;

        cpulock.unlock();

//        std::cout << req->runtime_remaining << std::endl;

        //TEST
//        ret += 3*slice; //see what happens
        res->rsrc_amnt = ret;   //set bw we're returning

////        res->rsrc_amnt = req->rsrc_amnt; //TODO: this just gives back what was asked for!
//        flag++;
        res->request = 0;       //set to give back
        return __ALLOC_SUCCESS__;
    }
    else {
        cpulock.unlock();
        //TODO: Throttle here
        res->rsrc_amnt = 0;
        res->request = 0;
        return __ALLOC_FAILED__;
    }
    */
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
    std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_c_id() << std::endl;
    res->request = 0; //giveback (or send back)
    return ret;
}


