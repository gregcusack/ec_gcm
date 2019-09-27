//
// Created by greg on 9/11/19.
//

#include "Manager.h"


ec::Manager::Manager(uint32_t _ec_id) : ec_id(_ec_id), s(nullptr) {}

ec::Manager::Manager(uint32_t _ec_id, int64_t _quota, uint64_t _slice_size)
    : ec_id(_ec_id), s(nullptr), quota(_quota), slice(_slice_size),
    runtime_remaining(_quota) {
    std::cout << "runtime_remaining on init: " << runtime_remaining << std::endl;

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

void ec::Manager::allocate_container(uint32_t cgroup_id, std::string server_ip) {

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
    return 1; //not sure what exactly to return here
}

uint64_t ec::Manager::decr_rt_remaining(uint64_t _slice) {
    runtime_remaining -= _slice;
    return runtime_remaining;
}

uint64_t ec::Manager::incr_rt_remaining(uint64_t give_back) {
    runtime_remaining += give_back;
    return runtime_remaining;
}

uint64_t ec::Manager::handle_bandwidth(msg_t *req) {
    uint64_t ret;
    std::mutex cpulock;
    if(req->req_type != _CPU_) { return __ALLOC_FAILED__; }
    cpulock.lock();
    if(runtime_remaining > 0) {
        ret = slice > runtime_remaining ? runtime_remaining : slice;
        runtime_remaining -= ret;
        cpulock.unlock();
        std::cout << "Server sending back " << ret << "ns in runtime" << std::endl;
        //TODO: THIS SHOULDN'T BE HERE. BUT USING IT FOR TESTING
        if(runtime_remaining <= 0) {
            refill_runtime();
        }
        return ret;
    }
    else {
        //TODO: Throttle here
        return __ALLOC_FAILED__;
    }

}

uint64_t ec::Manager::refill_runtime() {
    std::cout << "refilling runtime_remaining" << std::endl;
    runtime_remaining = quota;
    return runtime_remaining;
}

