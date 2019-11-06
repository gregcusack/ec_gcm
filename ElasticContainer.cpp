//
// Created by greg on 9/11/19.
//

#include "ElasticContainer.h"


ec::ElasticContainer::ElasticContainer(uint32_t _ec_id) : ec_id(_ec_id) {}

ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, std::vector<Agent *> &_agents)
    : ec_id(_ec_id), agents(_agents) {

    //TODO: change num_agents to however many servers we have. IDK how to set it rn.

    _mem = memory();
    _cpu = cpu();

    std::cout << "runtime_remaining on init: " << _cpu.runtime_remaining << std::endl;
    std::cout << "memory_available on init: " << _mem.memory_available << std::endl;

    test_file.open("test_file.txt", std::ios_base::app);

    //test
    flag = 0;

}

ec::SubContainer *ec::ElasticContainer::create_new_sc(const uint32_t cgroup_id, const uint32_t host_ip, const int sockfd) {
    return new SubContainer(cgroup_id, host_ip, ec_id, sockfd);
}

const ec::SubContainer &ec::ElasticContainer::get_subcontainer(ec::SubContainer::ContainerId &container_id) {
    auto itr = subcontainers.find(container_id);
    if(itr == subcontainers.end()) {
        std::cout << "ERROR: No EC with manager_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return *itr->second;
}

int ec::ElasticContainer::insert_sc(ec::SubContainer &_sc) {
    if (subcontainers.find(*_sc.get_id()) != subcontainers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        return __ALLOC_FAILED__;
    }
    subcontainers.insert({*(_sc.get_id()), &_sc});
    return __ALLOC_SUCCESS__;
}

uint64_t ec::ElasticContainer::refill_runtime() {
    std::cout << "refilling runtime_remaining" << std::endl;
    _cpu.runtime_remaining = _cpu.quota;
    return _cpu.runtime_remaining;
}

ec::ElasticContainer::~ElasticContainer() {
    for(auto &i : subcontainers) {
        delete i.second;
    }
    subcontainers.clear();
}

