//
// Created by greg on 9/11/19.
//

#include "ElasticContainer.h"


ec::ElasticContainer::ElasticContainer(uint32_t _ec_id) : ec_id(_ec_id), fair_cpu_share(0) {
//    _mem = global::stats::mem();
//    _cpu = global::stats::cpu();
}

ec::ElasticContainer::ElasticContainer(uint32_t _ec_id, std::vector<AgentClient *> &_agent_clients)
    : ec_id(_ec_id), fair_cpu_share(0) {

    //TODO: change num_agents to however many managers we have. IDK how to set it rn.

//    _mem = global::stats::mem();
//    _cpu = global::stats::cpu();

//    std::cout << "[Elastic Container Log] runtime_remaining on init: " << _cpu.get_runtime_remaining() << std::endl;
    std::cout << "[Elastic Container Log] unallocated_memory_in_pages on init: " << _mem.get_unallocated_memory_in_pages() << std::endl;

    subcontainers = subcontainer_map();
    sc_ac_map = subcontainer_agentclient_map();

}

ec::SubContainer *ec::ElasticContainer::create_new_sc(const uint32_t cgroup_id, const uint32_t host_ip, const int sockfd) {
    return new SubContainer(cgroup_id, host_ip, sockfd);
}

ec::SubContainer *ec::ElasticContainer::create_new_sc(uint32_t cgroup_id, uint32_t host_ip, int sockfd, uint64_t quota, uint32_t nr_throttled) {
    return new SubContainer(cgroup_id, host_ip, sockfd, quota, nr_throttled);
}

ec::SubContainer &ec::ElasticContainer::get_subcontainer(const ec::SubContainer::ContainerId &container_id) {
    auto itr = subcontainers.find(container_id);
    if(itr == subcontainers.end()) {
        std::cout << "ERROR: No EC with manager_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return *itr->second;
}

ec::SubContainer *ec::ElasticContainer::get_sc_for_update(ec::SubContainer::ContainerId &container_id) {
    auto itr = subcontainers.find(container_id);
    if(itr == subcontainers.end()) {
        std::cout << "ERROR: For EC: " << ec_id << ", no subcontainer with container_id: " << container_id << ". Exiting...(sc for update)." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return itr->second;
}

int ec::ElasticContainer::insert_sc(ec::SubContainer &_sc) {
    if (subcontainers.find(*_sc.get_c_id()) != subcontainers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        //TODO: should delete sc
        return __ALLOC_FAILED__;
    }
//    sc_map_lock.lock();
    subcontainers.insert({*(_sc.get_c_id()), &_sc});
//    sc_map_lock.unlock();
//    std::cout << "[EC LOG]: sc inserted: " << *_sc.get_c_id() << std::endl;
    return __ALLOC_INIT__;
}

void ec::ElasticContainer::get_sc_from_agent(const AgentClient* client, std::vector<SubContainer::ContainerId> &res) {
//    while(sc_ac_map.empty()) {}
    if (sc_ac_map.empty()) {
        std::cout << "ERROR: SC-AGENT Map is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    for (const auto &i: sc_ac_map) {
        if (i.second == client) {
            //res = i.first;
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
//    std::cout << "update fair share. (tot_cpu, # subconts): (" << _cpu.get_total_cpu() << ", " << subcontainers.size() << ")" << std::endl;
    if(subcontainers.empty()) {
        fair_cpu_share = _cpu.get_total_cpu();
        return;
    }
    fair_cpu_share = (uint64_t)(_cpu.get_total_cpu() / subcontainers.size());
}

int ec::ElasticContainer::ec_delete_from_subcontainers_map(const SubContainer::ContainerId &sc_id) {
    //todo: delete reduce pod fair share!
    auto itr = subcontainers.find(sc_id);
    if(itr != subcontainers.end()) {
        auto tmp = itr->second;
        subcontainers.erase(sc_id);
        delete tmp;
    }
    else {
        std::cerr << "[EC ERROR]: Can't find sc_id in subcontainers map! sc_id: " << sc_id << std::endl;
        return -1;
    }
    return 0;
}

uint64_t ec::ElasticContainer::get_sc_memory_limit_in_bytes(const ec::SubContainer::ContainerId &sc_id) {
    uint64_t ret = 0;
    auto *ac = get_corres_agent(sc_id);
    if(!ac) {
        std::cerr << "[ERROR] NO AgentClient found for container id: " << sc_id << " get_mem_limit_in_bytes()" <<  std::endl;
        return 0;
    }
    auto sc = get_subcontainer(sc_id);

//    std::cout << "docker id used:" <<  sc.get_docker_id() << std::endl;
    if(sc.get_docker_id().empty()) {
        std::cout << "docker_id is 0!" << std::endl;
        return 0;
    }
    ret = ec::Facade::MonitorFacade::CAdvisor::getContMemLimit(ac->get_agent_ip().to_string(), sc.get_docker_id());
    return ret;
}

uint64_t ec::ElasticContainer::get_tot_mem_alloc_in_pages() {
    uint64_t tot_mem_alloc = 0;
    uint64_t tmp;
    for(const auto &sc : get_subcontainers()) {
        tmp = get_sc_memory_limit_in_bytes(sc.first);
//        std::cout << "sc rx mem limit in bytes: " << tmp << std::endl;
        tot_mem_alloc += tmp;
    }
    return ceil(tot_mem_alloc/4096);
}

