//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_MSG_H
#define EC_GCM_MSG_H

#include <cstdint>
#include <iostream>

namespace ec {
    struct msg_t {
        msg_t()                         = default;
        msg_t(const msg_t&)             = default;
        msg_t& operator=(const msg_t&)  = default;
        msg_t(msg_t&&)                  = default;
        msg_t& operator=(msg_t&&)       = default;

        uint32_t            client_ip;      //ip container sending message is on
        uint32_t            cgroup_id;      //id of container on that server
        bool                is_mem;         //1: mem, 0: cpu
        uint64_t            rsrc_amnt;      //amount of resources (cpu/mem)
        bool                request;        //1: mem, 0: cpu


        std::ostream& operator<<(std::ostream& os_) {
            return os_  << client_ip << ","
                        << cgroup_id << ","
                        << is_mem << ","
                        << rsrc_amnt << ","
                        << request;
        }
    };
}

#endif //EC_GCM_MSG_H
