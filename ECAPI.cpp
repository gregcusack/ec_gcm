//
// Created by greg on 9/12/19.
//

#include "ECAPI.h"

ec::ECAPI::ECAPI(uint32_t _ec_id, std::vector<AgentClient *> &_agent_clients)
    : manager_id(_ec_id), agent_clients(_agent_clients) {
    _ec = new ElasticContainer(manager_id, agent_clients);
}


void ec::ECAPI::create_ec() {
    _ec = new ElasticContainer(manager_id, agent_clients);       //pass in gcm_ip for now
}

const ec::ElasticContainer& ec::ECAPI::get_elastic_container() const {
    if(_ec == nullptr) {
        std::cout << "[ERROR]: Must create _ec before accessing it" << std::endl;
        exit(EXIT_FAILURE);
    }
    return *_ec;
}

ec::ECAPI::~ECAPI() {
    delete _ec;
}

//int ec::ECAPI::handle_req(const char *buff_in, char *buff_out, uint32_t host_ip, int clifd) {
int ec::ECAPI::handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_req()" << std::endl;
        exit(EXIT_FAILURE);
    }

    uint64_t ret = __FAILED__;
    std::cout << "Req rx: " << *req << std::endl;

    switch(req -> req_type) {
        case _MEM_:
            ret = handle_mem_req(req, res, clifd);
            break;
        case _CPU_:
            ret = handle_cpu_usage_report(req, res);
            break;
        case _INIT_:
            ret = handle_add_cgroup_to_ec(req, res, host_ip, clifd);
            break;
        default:
            std::cout << "[Error]: ECAPI: " << manager_id << ". Handling memory/cpu request failed!" << std::endl;
            std::cout << "Req fail rx: " << *req << std::endl;
    }
    return ret;
}

void ec::ECAPI::ec_decrement_memory_available(uint64_t mem_to_reduce) {
    _ec->ec_decrement_memory_available(mem_to_reduce);
}





