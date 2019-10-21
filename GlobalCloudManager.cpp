//
// Created by greg on 9/12/19.
//

#include "GlobalCloudManager.h"

//ec::GlobalCloudManager::GlobalCloudManager()
//    : gcm_ip(ec::ip4_addr::from_string("127.0.0.1")), gcm_port(4444), ec_counter(1) {}

ec::GlobalCloudManager::GlobalCloudManager(std::string ip_addr, uint16_t port, agents_ip_list &_agent_ips, std::vector<uint16_t> &_ec_ports)
    : gcm_ip(ec::ip4_addr::from_string(std::move(ip_addr))), gcm_port(port), ec_ports(_ec_ports), ec_counter(1) {

    for(const auto &i : _agent_ips) {
        agents.emplace_back(new Agent(i));
    }
    for(auto &i : agents) {
        std::cout << "agent: " << *i << std::endl;
    }

    if(agents.size() != _agent_ips.size()) {
        std::cout << "ERROR: alloc agents failed!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

uint32_t ec::GlobalCloudManager::create_ec() {
    if(ecs.find(ec_counter) != ecs.end()) {
        std::cout << "ERROR: Error allocating new EC. EC IDs not correct" << std::endl;
        return 0;
    }

    uint16_t ec_port = ec_ports[ec_counter - 1]; //Give new EC, the next available port in the list
    auto *ec = new ec::ElasticContainer(ec_counter, gcm_ip, ec_port, agents);
    ecs.insert({ec_counter, ec});

//    eclock.lock();
    ec_counter++;
//    eclock.unlock();
    return ec->get_ec_id();
}

ec::ElasticContainer *ec::GlobalCloudManager::get_ec(uint32_t ec_id) {
    auto itr = ecs.find(ec_id);
    if(itr == ecs.end()) {
        std::cout << "ERROR: No EC with ec_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return itr->second;
}

ec::GlobalCloudManager::~GlobalCloudManager() {
    for(auto a : agents) {
        delete a;
    }
    agents.clear();
    for(auto &ec : ecs) {
        delete ec.second;
    }
    ecs.clear();
}