//
// Created by greg on 9/11/19.
//

#include "SubContainer.h"

ec::SubContainer::SubContainer(uint32_t cgroup_id, uint32_t ip, uint32_t manager_id) {
    _id = ContainerId(cgroup_id, ip4_addr::from_net(ip), manager_id);
    runtime_received = 0;
    mem_limit = 0;
    fd = 0;
}

ec::SubContainer::SubContainer(uint32_t cgroup_id, ip4_addr ip, uint32_t manager_id, int _fd)
    : fd(_fd) {

    _id = ContainerId(cgroup_id, ip, manager_id);
    runtime_received = 0;
    mem_limit = 0;
}

ec::SubContainer::SubContainer(uint32_t cgroup_id, ip4_addr ip, uint32_t manager_id, uint64_t mem_lim, int _fd)
    : mem_limit(mem_lim), fd(_fd) {

    _id = ContainerId(cgroup_id, ip, manager_id);
    runtime_received = 0;
}

ec::SubContainer::SubContainer(uint32_t cgroup_id, uint32_t ip, uint32_t manager_id, int _fd)
    : fd(_fd){
    _id = ContainerId(cgroup_id, ip4_addr::from_net(ip), manager_id);
    runtime_received = 0;
    mem_limit = 0;
}

ec::SubContainer::SubContainer(uint32_t cgroup_id, std::string ip, uint32_t manager_id, int _fd)
    : fd(_fd){
    _id = ContainerId(cgroup_id, ip4_addr::from_string(std::move(ip)), manager_id);
    runtime_received = 0;
    mem_limit = 0;
}

ec::SubContainer::SubContainer(uint32_t cgroup_id, std::string ip, uint32_t manager_id) {
    _id = ContainerId(cgroup_id, ip4_addr::from_string(std::move(ip)), manager_id);
    runtime_received = 0;
    mem_limit = 0;
    fd = 0;
}

ec::SubContainer::ContainerId::ContainerId(uint32_t _cgroup_id, ip4_addr _ip, uint32_t _ec_id)
    : cgroup_id(_cgroup_id), server_ip(_ip), ec_id(_ec_id) {}

bool ec::SubContainer::ContainerId::operator==(const ec::SubContainer::ContainerId &other_) const {
    return  cgroup_id   == other_.cgroup_id
            && server_ip == other_.server_ip
            && ec_id == other_.ec_id;
}