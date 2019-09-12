//
// Created by greg on 9/11/19.
//

#include "Manager.h"


ec::Manager::Manager(uint32_t _ec_id) : ec_id(_ec_id) {
    containers = {};
}

void ec::Manager::allocate_new_cgroup(uint32_t cgroup_id, uint32_t server_ip) {

    auto *c = new ec::SubContainer(cgroup_id, server_ip, ec_id);
    if (containers.find(*c->get_id()) != containers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        return;
    }
    std::cout << "Inserting (" << *c->get_id() << ")" << std::endl;
    containers.insert({*c->get_id(), c});
}

void ec::Manager::allocate_new_cgroup(uint32_t cgroup_id, std::string server_ip) {

    auto *c = new ec::SubContainer(cgroup_id, std::move(server_ip), ec_id);
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