//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"

ec::ECAPI::ECAPI(uint32_t _ec_id, std::vector<Agent *> &_agents)
    : manager_id(_ec_id), agents(_agents) {
    _ec = new ElasticContainer(manager_id, agents);
}


void ec::ECAPI::create_ec() {
    _ec = new ElasticContainer(manager_id, agents);       //pass in gcm_ip for now
}

const ec::ElasticContainer& ec::ECAPI::get_elastic_container() const {
    if(_ec == nullptr) {
        std::cout << "[ERROR]: Must create _ec before accessing it" << std::endl;
        exit(EXIT_FAILURE);
    }
    return *_ec;
}

ec::ECAPI::~ECAPI() {
    delete _ec;
}

int ec::ECAPI::handle_add_cgroup_to_ec(ec::msg_t *res, uint32_t cgroup_id, const uint32_t ip, int fd) {
    if(!res) {
        std::cout << "ERROR. res == null in handle_add_cgroup_to_ec()" << std::endl;
        return __ALLOC_FAILED__;
    }
    auto *sc = _ec->create_new_sc(cgroup_id, ip, fd);
    int ret = _ec->insert_sc(*sc);
    std::cout << "[dbg]: Init. Added cgroup to _ec. cgroup id: " << *sc->get_id() << std::endl;
    res->request = 0; //giveback (or send back)
    return ret;
}

void ec::ECAPI::ec_decrement_memory_available(uint64_t mem_to_reduce) {
    _ec->ec_decrement_memory_available(mem_to_reduce);
}





