//
// Created by greg on 9/11/19.
//

#include "SubContainer.h"


ec::SubContainer::SubContainer(uint32_t cgroup_id, uint32_t ip, int _fd)
    : fd(_fd), cpu(local::stats::cpu()), mem(local::stats::mem()), _docker_id("") {

    c_id = ContainerId(cgroup_id, ip4_addr::from_net(ip));
    counter = 0;
}

ec::SubContainer::SubContainer(uint32_t cgroup_id, uint32_t ip, int _fd, uint64_t _quota, uint32_t _nr_throttled)
    : fd(_fd), mem(local::stats::mem()), _docker_id("") {

    SPDLOG_DEBUG("creating sc with cg_id, quota, throttle: {}, {}, {}", cgroup_id, _quota, _nr_throttled);
    c_id = ContainerId(cgroup_id, ip4_addr::from_net(ip));
    cpu = local::stats::cpu(_quota, _nr_throttled);
    counter = 0;
}

uint32_t ec::SubContainer::get_thr_incr_and_set_thr(uint32_t _throttled) {
    auto incr = cpu.get_throttle_increase(_throttled);
    if(incr != 0) {
        cpu.set_throttled(_throttled);
    }
    return incr;
}

bool ec::SubContainer::ContainerId::operator==(const ec::SubContainer::ContainerId &other_) const {
    return  cgroup_id   == other_.cgroup_id
            && server_ip == other_.server_ip;
}