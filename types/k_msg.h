//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_K_MSG_H
#define EC_GCM_K_MSG_H

#include <cstdint>
#include <iostream>
#include "../om.h"

namespace ec {
    struct k_msg_t {
        k_msg_t(uint32_t client_ip, uint32_t cgroup_id, uint8_t is_mem, uint64_t rscr_amnt, uint8_t request)
                : client_ip(client_ip), cgroup_id(cgroup_id), is_mem(is_mem), rsrc_amnt(rscr_amnt), request(request){
        }
        uint32_t            client_ip;      //ip SubContainer sending message is on
        uint32_t            cgroup_id;      //id of SubContainer on that Server
        uint32_t                is_mem;         //1: mem, 0: cpu
        uint64_t            rsrc_amnt;      //amount of resources (cpu/mem)
        uint32_t                request;        //1: mem, 0: cpu


        friend std::ostream& operator<<(std::ostream& os_, const k_msg_t& k) {
            return os_  << "k_msg_t: "
                        << om::net::ip4_addr::from_net(k.client_ip) << ","
                        << k.cgroup_id << ","
                        << k.is_mem << ","
                        << k.rsrc_amnt << ","
                        << k.request;
        }

    };


}


#endif //EC_GCM_K_MSG_H
