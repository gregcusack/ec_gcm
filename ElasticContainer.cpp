//
// Created by greg on 9/11/19.
//

#include "ElasticContainer.h"


ec::ElasticContainer::ElasticContainer(uint32_t _ec_id) : ec_id(_ec_id) {}

ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, std::vector<AgentClient *> &_agent_clients)
    : ec_id(_ec_id), agent_clients(_agent_clients) {

    //TODO: change num_agents to however many servers we have. IDK how to set it rn.

    _mem = global::stats::mem();
    _cpu = global::stats::cpu();

    std::cout << "[Elastic Container Log] runtime_remaining on init: " << _cpu.get_runtime_remaining() << std::endl;
    std::cout << "[Elastic Container Log] memory_available on init: " << _mem.get_mem_available() << std::endl;

    test_file.open("test_file.txt", std::ios_base::app);

    //test
    flag = 0;
    subcontainers = subcontainer_map();
    sc_agent_map = subcontainer_agent_map();
}

ec::SubContainer *ec::ElasticContainer::create_new_sc(const uint32_t cgroup_id, const uint32_t host_ip, const int sockfd) {
    return new SubContainer(cgroup_id, host_ip, sockfd);
}

ec::SubContainer &ec::ElasticContainer::get_subcontainer(const ec::SubContainer::ContainerId &container_id) {
    auto itr = subcontainers.find(container_id);
    if(itr == subcontainers.end()) {
        std::cout << "ERROR: No EC with manager_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return *itr->second;
}

int ec::ElasticContainer::insert_sc(ec::SubContainer &_sc) {
    if (subcontainers.find(*_sc.get_c_id()) != subcontainers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        //TODO: should delete sc
        return __ALLOC_FAILED__;
    }
    subcontainers.insert({*(_sc.get_c_id()), &_sc});
    return __ALLOC_SUCCESS__;
}

void ec::ElasticContainer::get_sc_from_agent(AgentClient* client, std::vector<SubContainer::ContainerId> &res) {
    if (sc_agent_map.size() == 0) {
        std::cout << "ERROR: SC-AGENT Map is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    for (const auto &i: sc_agent_map) {
        if (i.second == client) {
            //res = i.first;
            res.push_back(i.first);
        }
    }
}

// int ec::ElasticContainer::int insert_pid(int pid){
//     if (pids == NULL) {
//         return __ALLOC_FAILED__;
//     }
//     pids.push_back(pid);
//     return __ALLOC_SUCCESS__;
// }

// int ec::ElasticContainer::int get_pids(){
//     if (pids == NULL) {
//         return __ALLOC_FAILED__;
//     }
//     return pids;
// }

uint64_t ec::ElasticContainer::refill_runtime() {
    return _cpu.refill_runtime();
}

ec::ElasticContainer::~ElasticContainer() {
    for(auto &i : subcontainers) {
        delete i.second;
    }
    subcontainers.clear();
}