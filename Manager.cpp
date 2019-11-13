//
// Created by Greg Cusack on 11/5/19.
//

#include "Manager.h"

int ec::Manager::handle_cpu_req(const ec::msg_t *req, ec::msg_t *res) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_cpu_req()" << std::endl;
        exit(EXIT_FAILURE);
    }
    uint64_t ret;
    std::mutex cpulock;
    if(req->req_type != _CPU_) { return __ALLOC_FAILED__; }
    cpulock.lock();
    uint64_t ec_rt_remaining = ec_get_cpu_runtime_remaining();
    if(ec_rt_remaining > 0) {
        //give back what it asks for
        ret = req->rsrc_amnt > ec_rt_remaining ? ec_rt_remaining : req->rsrc_amnt;

//        runtime_remaining -= ret; //TODO: put back in at some point??
//        std::cout << "Server sending back " << ret << "ns in runtime" << std::endl;
        //TODO: THIS SHOULDN'T BE HERE. BUT USING IT FOR TESTING
        if(ec_rt_remaining <= 0) {
            ec_refill_runtime();
        }
        cpulock.unlock();

        std::cout << req->request << "," << req->slice_succeed << "," << req->slice_fail << std::endl;

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

//        std::cout << "successfully decrease remaining mem to: " << memory_available << std::endl;

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
    int ret;

    std::cout << "[INFO] GCM: Trying to reclaim memory from other cgroups!" << std::endl;
    for (const auto &container : get_subcontainers()) {
        if (container.second->get_fd() == client_fd) {
            continue;
        }
        auto ip = container.second->get_c_id()->server_ip;
        std::cout << "ip of server container is on. also ip of agent_clients" << std::endl;
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
                if (strncmp(buffer, "NOMEM", sizeof("NOMEM")) != 0) {
                    reclaimed += *((uint64_t *) buffer);
                }
                std::cout << "[INFO] GCM: Current amount of reclaimed memory: " << reclaimed << std::endl;

            }

        }

    }
    std::cout << "[dbg] Recalimed memory at the end of the reclaim function: " << reclaimed << std::endl;
    return reclaimed;
}



