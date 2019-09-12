//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_CONTAINER_H
#define EC_GCM_CONTAINER_H

#include <cstdint>
#include <iostream>
//#include "manager.h"
//
class manager;
struct container_id;
//struct manager::container_id;
//struct container_id;

namespace ec {
    /*!
     * @class cg (cgroup)
     * @brief individual cgroup running on each server
     */
    class container {
    public:
        container(uint32_t cgroup_id, std::string ip, uint32_t manager_id);
        container(uint32_t cgroup_id, std::string ip, uint32_t manager_id, uint64_t mem_lim);

        struct container_id {
            container_id(uint32_t _cgroup_id, std::string _ip, uint32_t _ec_id);
            container_id() = default;
            uint32_t cgroup_id         = 0;
            std::string server_ip      = "";
            uint32_t ec_id              = 0;
            bool operator==(const container_id& other_) const;
            friend std::ostream& operator<<(std::ostream& os, const container_id& rhs) {
                os  << "cgroup_id: " << rhs.cgroup_id << ", "
                    << "server_ip: " << rhs.server_ip << ", "
                    << "ec_id: " << rhs.ec_id;
                return os;
            };
        };

        container_id* get_id() {return &_id;}


    private:
        container_id _id;
//        manager *manager_p;
        container_id *c_id;
        uint64_t runtime_received;
        uint64_t mem_limit;


    };


}
//
namespace std {
    template<>
    struct hash<ec::container::container_id> {
        std::size_t operator()(ec::container::container_id const& p) const {
            auto h1 = std::hash<uint32_t>()(p.cgroup_id);
            auto h2 = std::hash<std::string>()(p.server_ip);
            auto h3 = std::hash<uint32_t>()(p.ec_id);
            return h1 xor h2 xor h3;
        }
    };
}


//namespace ec {
//    struct ec_hash {
//        std::size_t operator()(ec::container::container_id const& p) const {
//            auto h1 = std::hash<uint32_t>()(p.cgroup_id);
//            std::cout << "sup: " << h1 << std::endl;
////            auto h2 = std::hash<std::string>()(p->server_ip);
////            auto h3 = std::hash<uint32_t>()(p->ec_id);
//            return h1;// xor h2 xor h3;
//        }
//    };
//}



#endif //EC_GCM_CONTAINER_H
