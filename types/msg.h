//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_MSG_H
#define EC_GCM_MSG_H

#include <cstdint>
#include <iostream>
#include <stdlib.h>
#include "../om.h"
#include "k_msg.h"

namespace ec {
    struct msg_t {
        msg_t()                         = default;
        msg_t(const msg_t&)             = default;
        msg_t& operator=(const msg_t&)  = default;
        msg_t(msg_t&&)                  = default;
        msg_t& operator=(msg_t&&)       = default;
        explicit msg_t(const ec::k_msg_t& k_msg);

        om::net::ip4_addr   client_ip;      //ip SubContainer sending message is on
//        uint32_t            client_ip;      //ip SubContainer sending message is on
        uint32_t            cgroup_id;      //id of SubContainer on that Server
        uint32_t                is_mem;         //1: mem, 0: cpu
        uint64_t            rsrc_amnt;      //amount of resources (cpu/mem)
        uint32_t                request;        //1: mem, 0: cpu


        friend std::ostream& operator<<(std::ostream& os_, const msg_t& k) {
            return os_  << "msg_t: "
                        << k.client_ip << ","
                        << k.cgroup_id << ","
                        << k.is_mem << ","
                        << k.rsrc_amnt << ","
                        << k.request;
        }


    };



}

#endif //EC_GCM_MSG_H
