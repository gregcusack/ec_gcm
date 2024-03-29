//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_MSG_H
#define EC_GCM_MSG_H

#include <cstdint>
#include <iostream>
#include <cstdlib>
#include <utility>
#include "../om.h"
#include <sys/socket.h>

namespace ec {
    struct msg_t {
        msg_t()                                     = default;
        msg_t(const msg_t &msg_req);
        msg_t& operator=(const msg_t&)              = default;
        msg_t(msg_t&&)                              = default;
        msg_t& operator=(msg_t&&)                   = default;

        om::net::ip4_addr   client_ip;      //ip SubContainer sending message is on
        uint32_t            cgroup_id;      //id of SubContainer on that Server
        uint32_t            req_type;         //0: cpu, 1: mem, 2: init, 3: slice, 4: create_cont
        uint64_t            rsrc_amnt;      //amount of resources (cpu/mem)
        uint32_t            request;        //1: request, 0: give back
        uint64_t            runtime_remaining;
        uint64_t            cpustat_seq_num;
        int64_t             sock_addr;

        friend std::ostream& operator<<(std::ostream& os_, const msg_t& k) {
            return os_ << "msg_t: "
                       << k.client_ip << ","
                       << k.cgroup_id << ","
                       << k.req_type << ","
                       << k.rsrc_amnt << ","
                       << k.request << ","
                       << k.runtime_remaining << ","
                       << k.cpustat_seq_num;
        }

        void from_net() {
            client_ip = om::net::ip4_addr::reverse_byte_order(client_ip);
        }

        void set_ip_from_net(uint32_t ip) { //this will not be needed later
            client_ip = om::net::ip4_addr::from_net(ip);
        };
        void set_ip_from_host(uint32_t ip) {
            client_ip = om::net::ip4_addr::from_host(ip);
        }
        void set_ip_from_string(std::string ip) { //this will not be needed later
            client_ip = om::net::ip4_addr::from_string(std::move(ip));
        };


    };

}

#endif //EC_GCM_MSG_H
