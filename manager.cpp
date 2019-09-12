//
// Created by greg on 9/11/19.
//

#include "manager.h"


ec::manager::manager(uint32_t _ec_id) : ec_id(_ec_id) {
    containers = {};
}

void ec::manager::allocate_new_cgroup(uint32_t cgroup_id, std::string server_ip) {

    auto *c = new ec::container(cgroup_id, std::move(server_ip), ec_id);
    if (containers.find(*c->get_id()) != containers.end()) {
        std::cout << "This container already exists! Can't allocate identical one!" << std::endl;
        return;
    }
    std::cout << "Inserting (" << *c->get_id() << ")" << std::endl;
    containers.insert({*c->get_id(), c});
}

uint32_t ec::manager::handle(uint32_t cgroup_id, std::string server_ip) {
    auto c_id = ec::container::container_id(cgroup_id, std::move(server_ip), ec_id);
    if (containers.find(c_id) != containers.end()) {
        return 0;
    }
    return 1;
}
