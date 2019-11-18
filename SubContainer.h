//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_CONTAINER_H
#define EC_GCM_CONTAINER_H

#include <cstdint>
#include <iostream>
#include "types/types.h"
#include "stats/local/cpu_l.h"
#include "stats/local/mem_l.h"

namespace ec {
    /*!
     * @class cg (cgroup)
     * @brief individual cgroup running on each server
     */
    class SubContainer {
    public:
        SubContainer(uint32_t cgroup_id, uint32_t ip, int fd);     //from uint32_t
        ~SubContainer() = default;

        struct ContainerId {
            ContainerId(uint32_t _cgroup_id, ip4_addr _ip)
                : cgroup_id(_cgroup_id), server_ip(_ip) {};
            ContainerId() = default;
            uint32_t cgroup_id          = 0;
            ip4_addr server_ip;
            bool operator==(const ContainerId& other_) const;
            friend std::ostream& operator<<(std::ostream& os, const ContainerId& rhs) {
                os  << "cgroup_id: " << rhs.cgroup_id << ", "
                    << "server_ip: " << rhs.server_ip;// << ", "
                return os;
            };
        };

        ContainerId* get_c_id() {return &c_id;}
        int get_fd() { return fd; }

    private:
        ContainerId c_id;
        int fd;

        local::stats::cpu cpu;
        local::stats::mem mem;


    };


}
//
namespace std {
    template<>
    struct hash<ec::SubContainer::ContainerId> {
        std::size_t operator()(ec::SubContainer::ContainerId const& p) const {
            auto h1 = std::hash<uint32_t>()(p.cgroup_id);
            auto h2 = std::hash<om::net::ip4_addr>()(p.server_ip);
            return h1 xor h2;// xor h3;
        }
    };
}




#endif //EC_GCM_CONTAINER_H
