//
// Created by greg on 9/12/19.
//


#include "msg.h"

ec::msg_t::msg_t(const ec::k_msg_t &k_msg) {
    client_ip = om::net::ip4_addr::from_host(k_msg.client_ip);
    cgroup_id = k_msg.cgroup_id;
    req_type = k_msg.is_mem;
    rsrc_amnt = k_msg.rsrc_amnt;
    request = k_msg.request;

}