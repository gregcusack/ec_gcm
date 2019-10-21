//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_CONTAINER_H
#define EC_GCM_CONTAINER_H

#include <cstdint>
#include <iostream>
#include "types/types.h"
//#include "Manager.h"
//
//class Manager;
//struct container_id;
//struct Manager::ContainerId;
//struct ContainerId;

namespace ec {
    /*!
     * @class cg (cgroup)
     * @brief individual cgroup running on each server
     */
    class SubContainer {
    public:
        SubContainer(uint32_t cgroup_id, uint32_t ip, uint32_t manager_id);
        SubContainer(uint32_t cgroup_id, std::string ip, uint32_t manager_id);
        SubContainer(uint32_t cgroup_id, uint32_t ip, uint32_t manager_id, int fd);     //from uint32_t
        SubContainer(uint32_t cgroup_id, ip4_addr ip, uint32_t manager_id, int fd);     //from ip4_addr
        SubContainer(uint32_t cgroup_id, std::string ip, uint32_t manager_id, int fd);     //from const char[8]
        SubContainer(uint32_t cgroup_id, ip4_addr ip, uint32_t manager_id, uint64_t mem_lim, int fd);
        ~SubContainer() = default;

        struct ContainerId {
            ContainerId(uint32_t _cgroup_id, ip4_addr _ip, uint32_t _ec_id);
            ContainerId() = default;
            uint32_t cgroup_id          = 0;
            ip4_addr server_ip;
            uint32_t ec_id              = 0;
            bool operator==(const ContainerId& other_) const;
            friend std::ostream& operator<<(std::ostream& os, const ContainerId& rhs) {
                os  << "cgroup_id: " << rhs.cgroup_id << ", "
                    << "server_ip: " << rhs.server_ip << ", "
                    << "ec_id: " << rhs.ec_id;
                return os;
            };
        };

        //TODO: implement mem and cpu stats/current usage
//        struct cpu_stats {
//
//        };
//
//        struct mem_stats {
//
//        };

        ContainerId* get_id() {return &_id;}
        int get_fd() { return fd; }


    private:
        ContainerId _id;
        int fd;
//        Manager *manager_p;
        ContainerId *c_id;
        uint64_t runtime_received;
        uint64_t mem_limit;


    };


}
//
namespace std {
    template<>
    struct hash<ec::SubContainer::ContainerId> {
        std::size_t operator()(ec::SubContainer::ContainerId const& p) const {
            auto h1 = std::hash<uint32_t>()(p.cgroup_id);
            auto h2 = std::hash<om::net::ip4_addr>()(p.server_ip);
            auto h3 = std::hash<uint32_t>()(p.ec_id);
            return h1 xor h2 xor h3;
        }
    };
}




#endif //EC_GCM_CONTAINER_H
