//
// Created by greg on 9/11/19.
//

#include "ElasticContainer.h"


ec::ElasticContainer::ElasticContainer(uint32_t _ec_id) : ec_id(_ec_id) {}

ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, std::vector<Agent *> &_agents)
    : ec_id(_ec_id), agents(_agents) {

    //TODO: change num_agents to however many servers we have. IDK how to set it rn.
    std::cout << "runtime_remaining on init: " << runtime_remaining << std::endl;
    std::cout << "memory_available on init: " << memory_available << std::endl;

    _mem = memory();
    _cpu = cpu();

    test_file.open("test_file.txt", std::ios_base::app);

    //test
    flag = 0;

}

//void ec::ElasticContainer::allocate_container(uint32_t cgroup_id, uint32_t server_ip) {
//
//    auto *c = new ec::SubContainer(cgroup_id, server_ip, ec_id);
//    if (containers.find(*c->get_id()) != containers.end()) {
//        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
//        return;
//    }
//    std::cout << "Inserting (" << *c->get_id() << ")" << std::endl;
//    containers.insert({*c->get_id(), c});
//}
//
//void ec::ElasticContainer::allocate_container(uint32_t cgroup_id, const std::string &server_ip) {
//
//    auto *c = new ec::SubContainer(cgroup_id, server_ip, ec_id);
//    if (containers.find(*c->get_id()) != containers.end()) {
//        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
//        return;
//    }
//    std::cout << "Inserting (" << *c->get_id() << ")" << std::endl;
//    containers.insert({*c->get_id(), c});
//}


ec::SubContainer *ec::ElasticContainer::create_new_sc(const uint32_t cgroup_id, const uint32_t host_ip, const int sockfd) {
    return new SubContainer(cgroup_id, host_ip, ec_id, sockfd);
}

//uint32_t ec::ElasticContainer::handle(uint32_t cgroup_id, uint32_t server_ip) {
//    auto c_id = ec::SubContainer::ContainerId(cgroup_id, ip4_addr::from_host(server_ip), ec_id);
//    if (containers.find(c_id) != containers.end()) {
//        return 0;
//    }
//    return 1;
//}

ec::SubContainer *ec::ElasticContainer::get_container(ec::SubContainer::ContainerId &container_id) {
    auto itr = containers.find(container_id);
    if(itr == containers.end()) {
        std::cout << "ERROR: No EC with manager_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return itr->second;
}

int ec::ElasticContainer::insert_sc(ec::SubContainer &_sc) {
    if (containers.find(*_sc.get_id()) != containers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        return __ALLOC_FAILED__;
    }
    containers.insert({*(_sc.get_id()), &_sc});
    return __ALLOC_SUCCESS__;
}

uint64_t ec::ElasticContainer::refill_runtime() {
    std::cout << "refilling runtime_remaining" << std::endl;
    runtime_remaining = quota;
    return runtime_remaining;
}

ec::ElasticContainer::~ElasticContainer() {
    for(auto &i : containers) {
        delete i.second;
    }
    containers.clear();
}

