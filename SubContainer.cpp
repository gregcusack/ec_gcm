//
// Created by greg on 9/11/19.
//

#include "SubContainer.h"


ec::SubContainer::SubContainer(uint32_t cgroup_id, uint32_t ip, int _fd)
    : fd(_fd), cpu(local::stats::cpu()), mem(local::stats::mem()) {
    c_id = ContainerId(cgroup_id, ip4_addr::from_net(ip));
    
    cpu = local::stats::cpu();
    mem = local::stats::mem();

}

bool ec::SubContainer::ContainerId::operator==(const ec::SubContainer::ContainerId &other_) const {
    return  cgroup_id   == other_.cgroup_id
            && server_ip == other_.server_ip;
}