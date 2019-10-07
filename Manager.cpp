//
// Created by greg on 9/11/19.
//

#include "Manager.h"


ec::Manager::Manager(uint32_t _ec_id) : ec_id(_ec_id), s(nullptr) {}

ec::Manager::Manager(uint32_t _ec_id, int64_t _quota, uint64_t _slice_size,
        uint64_t _mem_limit, uint64_t _mem_slice_size)
    : ec_id(_ec_id), s(nullptr), quota(_quota), slice(_slice_size),
      runtime_remaining(_quota), memory_available(_mem_limit), mem_slice(_mem_slice_size) {
    std::cout << "runtime_remaining on init: " << runtime_remaining << std::endl;
    std::cout << "memory_available on init: " << memory_available << std::endl;

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
        ret = slice > runtime_remaining ? runtime_remaining : slice;
        runtime_remaining -= ret;
//        std::cout << "Server sending back " << ret << "ns in runtime" << std::endl;
        //TODO: THIS SHOULDN'T BE HERE. BUT USING IT FOR TESTING
        if(runtime_remaining <= 0) {
            refill_runtime();
        }
        cpulock.unlock();
        res->rsrc_amnt = ret;   //set bw we're returning
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

int ec::Manager::handle_mem_req(const ec::msg_t *req, ec::msg_t *res) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_mem_req()" << std::endl;
        exit(EXIT_FAILURE);
    }
    uint64_t ret = 0;
    std::mutex memlock;
    if(req->req_type != _MEM_) { return __ALLOC_FAILED__; }
    memlock.lock();
    if(memory_available > 0) {          //TODO: integrate give back here
//        std::cout << "Handle mem req: success. memory available: " << memory_available << std::endl;
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
//        std::cout << "no memory available" << std::endl;
        res->rsrc_amnt = 0;
        return __ALLOC_FAILED__;
    }
}

int ec::Manager::handle_add_cgroup_to_ec(msg_t *res, const uint32_t cgroup_id, const uint32_t ip) {
    if(!res) {
        std::cout << "ERROR. res == null in handle_Add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }
    auto *sc = new SubContainer(cgroup_id, cgroup_id, ec_id);
    int ret = insert_sc(*sc);
    std::cout << "[dbg]: Init. Added cgroup to ec. cgroup id: " << *sc->get_id() << std::endl;
    res->request = 0; //giveback (or send back)
    return ret;
}
