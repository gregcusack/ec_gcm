//
// Created by greg on 9/11/19.
//

#include "ElasticContainer.h"


ec::ElasticContainer::ElasticContainer(uint32_t _ec_id) : ec_id(_ec_id), fair_cpu_share(0) { }

ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, std::vector<rpc::AgentClient *> &_agent_clients)
    : ec_id(_ec_id), fair_cpu_share(0) {

    SPDLOG_DEBUG("unallocated_memory_in_pages on init: {}", _mem.get_unallocated_memory_in_pages());
    subcontainers = subcontainer_map();
    sc_ac_map = subcontainer_agentclient_map();

}

ec::SubContainer *ec::ElasticContainer::create_new_sc(const uint32_t cgroup_id, const uint32_t host_ip, const int sockfd) {
    return new SubContainer(cgroup_id, host_ip, sockfd);
}

ec::SubContainer *ec::ElasticContainer::create_new_sc(uint32_t cgroup_id, uint32_t host_ip, int sockfd, uint64_t quota, uint32_t nr_throttled) {
    return new SubContainer(cgroup_id, host_ip, sockfd, quota, nr_throttled);
}

////// Get oldest subcontainer with the cg_id provided
ec::SubContainer &ec::ElasticContainer::get_subcontainer_front(const ec::SubContainer::ContainerId &container_id) {
    auto itr = subcontainers.find(container_id);
    if(itr == subcontainers.end()) {
        SPDLOG_CRITICAL("No EC with manager_id: {}. Exiting....", ec_id);
        std::exit(EXIT_FAILURE);
    }
    return *itr->second->front();
}

////// Get most recent container (current)
ec::SubContainer *ec::ElasticContainer::get_sc_for_update_back(const ec::SubContainer::ContainerId &container_id) {
    auto itr = subcontainers.find(container_id);
    if(itr == subcontainers.end()) {
        SPDLOG_CRITICAL("For EC: {}, no subcontainer with container_id: {}. Exiting...(sc for update).", ec_id, container_id);
        std::exit(EXIT_FAILURE);
    }
    return itr->second->back();
}

int ec::ElasticContainer::insert_sc(ec::SubContainer &_sc) {
    auto inserted = subcontainers.find(*_sc.get_c_id());
    if(inserted != subcontainers.end()) { //already inserted. add to queue
        auto q = inserted->second;
        q->push(&_sc);
        subcontainers.insert({*_sc.get_c_id(), q});
    }
    else { // not already inserted. create queue
        auto q =  new std::queue<SubContainer *>();// (&_sc);
        q->push(&_sc);
        subcontainers.insert({*_sc.get_c_id(), q});
    }
    return __ALLOC_INIT__;
}

void ec::ElasticContainer::get_sc_from_agent(const rpc::AgentClient* client, std::vector<SubContainer::ContainerId> &res) {
    if (sc_ac_map.empty()) {
        SPDLOG_CRITICAL("ERROR: SC-AGENT Map is empty");
        std::exit(EXIT_FAILURE);
    }

    for (const auto &i: sc_ac_map) {
        if (i.second == client) {
            res.push_back(i.first);
        }
    }
}

ec::ElasticContainer::~ElasticContainer() {
    for(auto &i : subcontainers) {
        delete i.second;
    }
    subcontainers.clear();
}
void ec::ElasticContainer::update_fair_cpu_share() {
    SPDLOG_TRACE("update fair share. (tot_cpu, # subconts): ({}, {})", _cpu.get_total_cpu(), subcontainers.size());
    if(subcontainers.empty()) {
        fair_cpu_share = _cpu.get_total_cpu();
        return;
    }
    fair_cpu_share = (uint64_t)(_cpu.get_total_cpu() / subcontainers.size());
}

int ec::ElasticContainer::ec_delete_from_subcontainers_map(const SubContainer::ContainerId &sc_id) {
    auto itr = subcontainers.find(sc_id);
    if(itr != subcontainers.end()) {
        if(itr->second->size() > 1) {   //size queue > 1, delete element at front of queue (oldest element)
            auto cont = itr->second->front();
            itr->second->pop();
            delete cont;
        }
        else {                          // if size queue == 1, delete back and delete queue
            auto q = itr->second;
            subcontainers.erase(sc_id);
            delete q;
        }
    }
    else {
        SPDLOG_ERROR("Can't find sc_id in subcontainers map! sc_id: {}", sc_id);
        return -1;
    }
    return 0;
}

uint64_t ec::ElasticContainer::get_sc_memory_limit_in_bytes(const ec::SubContainer::ContainerId &sc_id) {
    uint64_t ret = 0;
    auto *ac = get_corres_agent(sc_id);
    if(!ac) {
        SPDLOG_ERROR("NO AgentClient found for container id: {}. get_mem_limit_in_bytes()", sc_id);
        return 0;
    }

    if(sc_id.docker_id.empty()) {
        SPDLOG_ERROR("docker_id is 0!");
        return 0;
    }
    ret = ec::Facade::MonitorFacade::CAdvisor::getContMemLimit(ac->get_agent_ip().to_string(), sc_id.docker_id);
    return ret;
}

uint64_t ec::ElasticContainer::get_sc_memory_usage_in_bytes(const ec::SubContainer::ContainerId &sc_id) {
    uint64_t ret = 0;
    auto *ac = get_corres_agent(sc_id);
    if(!ac) {
        SPDLOG_ERROR("NO AgentClient found for container id: {}", sc_id);
        return 0;
    }

    if(sc_id.docker_id.empty()) {
        SPDLOG_ERROR("docker_id is 0!");
        return 0;
    }
    ret = ec::Facade::MonitorFacade::CAdvisor::getContMemUsage(ac->get_agent_ip().to_string(), sc_id.docker_id);
    return ret;
}


uint64_t ec::ElasticContainer::get_tot_mem_alloc_in_pages() {
    uint64_t tot_mem_alloc = 0;
    uint64_t tmp;
    for(const auto &sc : get_subcontainers()) {
        tmp = get_sc_memory_limit_in_bytes(sc.first);
        tot_mem_alloc += tmp;
    }
    return ceil(tot_mem_alloc/4096);
}

ec::SubContainer &ec::ElasticContainer::get_subcontainer_back(const ec::SubContainer::ContainerId &container_id) {
    auto itr = subcontainers.find(container_id);
    if(itr == subcontainers.end()) {
        SPDLOG_CRITICAL("No EC with manager_id: {}. Exiting....", ec_id);
        std::exit(EXIT_FAILURE);
    }
    return *itr->second->back();
}

