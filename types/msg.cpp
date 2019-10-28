//
// Created by greg on 9/12/19.
//


#include "msg.h"

ec::msg_t::msg_t(const ec::k_msg_t &k_msg) {
    client_ip = om::net::ip4_addr::from_host(k_msg.client_ip);
    cgroup_id = k_msg.cgroup_id;
    req_type = k_msg.req_type;
    rsrc_amnt = k_msg.rsrc_amnt;
    request = k_msg.request;

}

ec::msg_t::msg_t(const ec::msg_t &msg_req) {
    ec_id = msg_req.ec_id;
    client_ip = msg_req.client_ip;
    cgroup_id = msg_req.cgroup_id;
    req_type = msg_req.req_type;
    rsrc_amnt = msg_req.rsrc_amnt;
    request = msg_req.request;
    slice_succeed = msg_req.slice_succeed;
    slice_fail = msg_req.slice_fail;
}
