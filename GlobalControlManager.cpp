//
// Created by greg on 9/12/19.
//

#include "GlobalControlManager.h"

ec::GlobalControlManager::GlobalControlManager(std::string ip_addr, uint16_t port, agents_ip_list &_agent_ips, std::vector<uint16_t> &_server_ports)
    : gcm_ip(ec::ip4_addr::from_string(std::move(ip_addr))), mngr(nullptr), gcm_port(port), server_ports(_server_ports), manager_counts(1) {

    for(const auto &i : _agent_ips) {
        agents.emplace_back(new Agent(i));
    }

    if(agents.size() != _agent_ips.size()) {
        SPDLOG_ERROR("ERROR: alloc agents failed!");
        exit(EXIT_FAILURE);
    }
}

uint32_t ec::GlobalControlManager::create_manager() {
    if(managers.find(manager_counts) != managers.end()) {
        SPDLOG_ERROR("Error allocating new Server. Server IDs not correct");
        return 0;
    }

    uint16_t server_port = server_ports[manager_counts - 1]; //Give new EC, the next available port in the list
    mngr = new Manager(manager_counts, gcm_ip, server_port, agents);

    managers.insert({manager_counts, mngr});

    manager_counts++;
    return mngr->get_server_id();
}

const ec::Manager &ec::GlobalControlManager::get_manager(const int manager_id) const {
    auto itr = managers.find(manager_id);
    if(itr == managers.end()) {
        SPDLOG_CRITICAL(" No Server with manager_id: {}. Exiting....", manager_id);
        std::exit(EXIT_FAILURE);
    }
    return *itr->second;
}


ec::GlobalControlManager::~GlobalControlManager() {
    for(auto a : agents) {
        delete a;
    }
    agents.clear();
    for(auto &s : managers) {
        delete s.second;
    }
    managers.clear();
}

void ec::GlobalControlManager::run(const std::string &app_name, const std::string &_gcm_ip) {

    for(const auto &s : managers) {
        if(fork() == 0) {
            SPDLOG_TRACE("New Server with ID: {}", s.second->get_server_id());
            s.second->start(app_name, _gcm_ip);
        }
        else {
            continue;
        }
        break;
    }
    for(const auto &i : managers) {
        wait(nullptr);
    }
}

