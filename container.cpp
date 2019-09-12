//
// Created by greg on 9/11/19.
//

#include "container.h"

ec::container::container(uint32_t cgroup_id, std::string ip, uint32_t manager_id) {

    _id = container_id(cgroup_id, std::move(ip), manager_id);
    runtime_received = 0;
    mem_limit = 0;
}

ec::container::container(uint32_t cgroup_id, std::string ip, uint32_t manager_id, uint64_t mem_lim)
    : mem_limit(mem_lim) {

    _id = container_id(cgroup_id, std::move(ip), manager_id);
    runtime_received = 0;
}

ec::container::container_id::container_id(uint32_t _cgroup_id, std::string _ip, uint32_t _ec_id)
    : cgroup_id(_cgroup_id), server_ip(std::move(_ip)), ec_id(_ec_id) {}

bool ec::container::container_id::operator==(const ec::container::container_id &other_) const {
    return  cgroup_id   == other_.cgroup_id
            && server_ip == other_.server_ip
            && ec_id == other_.ec_id;
}