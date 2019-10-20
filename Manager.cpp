//
// Created by greg on 9/11/19.
//

#include "Manager.h"


ec::Manager::Manager(uint32_t _ec_id) : ec_id(_ec_id), s(nullptr) {}

ec::Manager::Manager(uint32_t _ec_id, std::vector<Agent *> &_agents, int64_t _quota, uint64_t _slice_size,
        uint64_t _mem_limit, uint64_t _mem_slice_size)
    : ec_id(_ec_id), agents(_agents), s(nullptr), quota(_quota), slice(_slice_size),
      runtime_remaining(_quota), memory_available(_mem_limit), mem_slice(_mem_slice_size) {

    //TODO: change num_agents to however many servers we have. IDK how to set it rn.
    std::cout << "runtime_remaining on init: " << runtime_remaining << std::endl;
    std::cout << "memory_available on init: " << memory_available << std::endl;

    //test
    flag = 0;

}

void ec::Manager::allocate_container(uint32_t cgroup_id, uint32_t server_ip) {

    auto *c = new ec::SubContainer(cgroup_id, server_ip, ec_id);
    if (containers.find(*c->get_id()) != containers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        return;
    }
    std::cout << "Inserting (" << *c->get_id() << ")" << std::endl;
    containers.insert({*c->get_id(), c});
}

void ec::Manager::allocate_container(uint32_t cgroup_id, const std::string &server_ip) {

    auto *c = new ec::SubContainer(cgroup_id, server_ip, ec_id);
    if (containers.find(*c->get_id()) != containers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        return;
    }
    std::cout << "Inserting (" << *c->get_id() << ")" << std::endl;
    containers.insert({*c->get_id(), c});
}

uint32_t ec::Manager::handle(uint32_t cgroup_id, uint32_t server_ip) {
    auto c_id = ec::SubContainer::ContainerId(cgroup_id, ip4_addr::from_host(server_ip), ec_id);
    if (containers.find(c_id) != containers.end()) {
        return 0;
    }
    return 1;
}

ec::SubContainer *ec::Manager::get_container(ec::SubContainer::ContainerId &container_id) {
    auto itr = containers.find(container_id);
    if(itr == containers.end()) {
        std::cout << "ERROR: No EC with ec_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return itr->second;
}

int ec::Manager::insert_sc(ec::SubContainer &_sc) {
    containers.insert({*(_sc.get_id()), &_sc});
    return __ALLOC_SUCCESS__; //not sure what exactly to return here
}

uint64_t ec::Manager::decr_rt_remaining(uint64_t _slice) {
    runtime_remaining -= _slice;
    return runtime_remaining;
}

uint64_t ec::Manager::incr_rt_remaining(uint64_t give_back) {
    runtime_remaining += give_back;
    return runtime_remaining;
}

uint64_t ec::Manager::refill_runtime() {
    std::cout << "refilling runtime_remaining" << std::endl;
    runtime_remaining = quota;
    return runtime_remaining;
}

int ec::Manager::handle_bandwidth(const msg_t *req, msg_t *res) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_bandwidth()" << std::endl;
        exit(EXIT_FAILURE);
    }
    uint64_t ret;
    std::mutex cpulock;
    if(req->req_type != _CPU_) { return __ALLOC_FAILED__; }
    cpulock.lock();
    if(runtime_remaining > 0) {
        //give back what it asks for
        ret = req->rsrc_amnt > runtime_remaining ? runtime_remaining : req->rsrc_amnt;

        runtime_remaining -= ret;
//        std::cout << "Server sending back " << ret << "ns in runtime" << std::endl;
        //TODO: THIS SHOULDN'T BE HERE. BUT USING IT FOR TESTING
        if(runtime_remaining <= 0) {
            refill_runtime();
        }
        cpulock.unlock();

        //TEST
        ret += 3*slice; //see what happens
        res->rsrc_amnt = ret;   //set bw we're returning
//        if(flag < 150) {
//            res->rsrc_amnt = req->rsrc_amnt; //TODO: this just gives back what was asked for!
//        }
//        else {
//            res->rsrc_amnt = 0;
//            if(flag == 499) flag = -1;
//        }
//
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

int ec::Manager::handle_slice_req(const ec::msg_t *req, ec::msg_t *res, int clifd) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_slice_req()" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(req->rsrc_amnt != slice) {
        std::cout << "req slice size != default slice of 5ms (req->rsrc_amnt, slice): "
                     "(" << req->rsrc_amnt << ", " << slice << ")" << std::endl;
    }
    std::cout << "pre  change req: " << *req << std::endl;
    std::cout << "pre change res: " << *res << std::endl;
    if(runtime_remaining > 0) {
        res->rsrc_amnt = std::min(runtime_remaining, slice);
        runtime_remaining -= res->rsrc_amnt;
        if(runtime_remaining <= 0) {
            refill_runtime();
        }
        res->request = 0;
        std::cout << "post  change req: " << *req << std::endl;
        std::cout << "post change res: " << *res << std::endl;
        return __ALLOC_SUCCESS__;
    }
    else {
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
    if(memory_available > 0 || (memory_available = reclaim_memory(clifd)) > 0) {          //TODO: integrate give back here
        std::cout << "Handle mem req: success. memory available: " << memory_available << std::endl;
        ret = memory_available > mem_slice ? mem_slice : memory_available;

        memory_available -= ret;

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

int ec::Manager::handle_add_cgroup_to_ec(msg_t *res, const uint32_t cgroup_id, const uint32_t ip, int fd) {
    if(!res) {
        std::cout << "ERROR. res == null in handle_Add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }
    auto *sc = new SubContainer(cgroup_id, ip, ec_id, fd);
    int ret = insert_sc(*sc);
    std::cout << "[dbg]: Init. Added cgroup to ec. cgroup id: " << *sc->get_id() << std::endl;
    res->request = 0; //giveback (or send back)
    return ret;
}

const std::vector<ec::Agent *> &ec::Manager::get_agents() const {
    return agents;
}

//int ec::Manager::alloc_agents(std::vector<Agent *> &_agents) {
//    for(auto agent_ip : agents_ips)
//        agents.emplace_back(new agent(agent_ip));
//
//    return agents.size();
//}

uint64_t ec::Manager::reclaim_memory(int client_fd) {
    int j = 0;
    char buffer[__BUFF_SIZE__] = {0};
    uint64_t reclaimed = 0;
    int ret;

    std::cout << "[INFO] GCM: Trying to reclaim memory from other cgroups!" << std::endl;
    for(const auto &container : containers) {
        if(container.second->get_fd() == client_fd) {
            continue;
        }
        auto ip = container.second->get_id()->server_ip;
        std::cout << "ip of server container is on. also ip of agent" << std::endl;

        for(const auto & ag : agents) {
            std::cout << "(ag->ip, container ip): (" << ag->get_ip() << ", " << ip << ")" << std::endl;
            if(ag->get_ip() == ip) {
                auto *reclaim_req = new reclaim_msg;
                reclaim_req->cgroup_id = container.second->get_id()->cgroup_id;
                reclaim_req->is_mem = 1;
                //TODO: anyway to get the server to do this?
                if (write(ag->get_sockfd(), (char *) reclaim_req, sizeof(*reclaim_req)) < 0) {
                    std::cout << "[ERROR]: GCM EC Server id: " << ec_id << ". Failed writing to agent socket"
                              << std::endl;
                }
                ret = read(ag->get_sockfd(), buffer, sizeof(buffer));
                if(ret <= 0) {
                    std::cout << "[ERROR]: GCM. Can't read from socke to reclaim memory" << std::endl;
                }
                if(strncmp(buffer, "NOMEM", sizeof("NOMEM")) != 0) {
                    reclaimed += *((uint64_t*)buffer);
                }
                std::cout << "[INFO] GCM: Current amount of reclaimed memory: " << reclaimed << std::endl;

            }

        }

    }
    std::cout << "[dbg] Recalimed memory at the end of the reclaim function: " << reclaimed << std::endl;
    return reclaimed;
}


