//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_MSG_H
#define EC_GCM_MSG_H

#include <cstdint>
#include <iostream>
#include <cstdlib>
#include "../om.h"
#include "k_msg.h"

namespace ec {
    struct msg_t {
        msg_t()                                     = default;
        msg_t(const msg_t &msg_req);
        msg_t& operator=(const msg_t&)              = default;
        msg_t(msg_t&&)                              = default;
        msg_t& operator=(msg_t&&)                   = default;
        explicit msg_t(const ec::k_msg_t& k_msg);

        uint32_t            ec_id;
        om::net::ip4_addr   client_ip;      //ip SubContainer sending message is on
//        uint32_t            client_ip;      //ip SubContainer sending message is on
        uint32_t            cgroup_id;      //id of SubContainer on that Server
        uint32_t                req_type;         //1: mem, 0: cpu, 2: init, 3: slice
        uint64_t            rsrc_amnt;      //amount of resources (cpu/mem)
        uint32_t                request;        //1: request, 0: give back
        uint32_t            slice_succeed;
        uint32_t            slice_fail;


        friend std::ostream& operator<<(std::ostream& os_, const msg_t& k) {
            return os_ << "msg_t: "
                       << k.ec_id << ","
                       << k.client_ip << ","
                       << k.cgroup_id << ","
                       << k.req_type << ","
                       << k.rsrc_amnt << ","
                       << k.request << ","
                       << k.slice_succeed << ","
                       << k.slice_fail;
        }

        void from_net() {
            client_ip = om::net::ip4_addr::reverse_byte_order(client_ip);
        }

        void set_ip(uint32_t ip) { //this will not be needed later
            client_ip = om::net::ip4_addr::from_net(ip);
        };


    };

}

#endif //EC_GCM_MSG_H
