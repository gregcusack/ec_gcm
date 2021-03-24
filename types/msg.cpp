//
// Created by greg on 9/12/19.
//


#include "msg.h"

ec::msg_t::msg_t(const ec::msg_t &msg_req) {
    client_ip = msg_req.client_ip;
    cgroup_id = msg_req.cgroup_id;
    req_type = msg_req.req_type;
    rsrc_amnt = msg_req.rsrc_amnt;
    request = msg_req.request;
    runtime_remaining = msg_req.runtime_remaining;
    cpustat_seq_num = msg_req.cpustat_seq_num;
//    cont_name = msg_req.cont_name;
}
