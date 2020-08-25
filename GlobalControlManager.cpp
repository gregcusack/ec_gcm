//
// Created by greg on 9/12/19.
//

#include "GlobalControlManager.h"

//_ec::GlobalControlManager::GlobalControlManager()
//    : gcm_ip(_ec::ip4_addr::from_string("127.0.0.1")), gcm_port(4444), manager_counts(1) {}

ec::GlobalControlManager::GlobalControlManager(std::string ip_addr, uint16_t port, agents_ip_list &_agent_ips, std::vector<uint16_t> &_server_ports)
    : gcm_ip(ec::ip4_addr::from_string(std::move(ip_addr))), mngr(nullptr), gcm_port(port), server_ports(_server_ports), manager_counts(1) {

    for(const auto &i : _agent_ips) {
        agents.emplace_back(new Agent(i));
    }
    // for(auto &i : agents) {
    //     std::cout << "agent_clients: " << *i << std::endl;
    // }

    if(agents.size() != _agent_ips.size()) {
        std::cout << "ERROR: alloc agents failed!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

uint32_t ec::GlobalControlManager::create_manager() {
    if(managers.find(manager_counts) != managers.end()) {
        std::cout << "ERROR: Error allocating new Server. Server IDs not correct" << std::endl;
        return 0;
    }

    uint16_t server_port = server_ports[manager_counts - 1]; //Give new EC, the next available port in the list
    mngr = new Manager(manager_counts, gcm_ip, server_port, agents);

    managers.insert({manager_counts, mngr});

//    eclock.lock();
    manager_counts++;
//    eclock.unlock();
    return mngr->get_server_id();
}

const ec::Manager &ec::GlobalControlManager::get_manager(const int manager_id) const {
    auto itr = managers.find(manager_id);
    if(itr == managers.end()) {
        std::cout << "ERROR: No Server with manager_id: " << manager_id << ". Exiting...." << std::endl;
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

    std::thread threads[__NUM_THREADS__];
    //app_thread_args *args;

    for(const auto &s : managers) {
        if(fork() == 0) {
            // std::cout << "[child] pid: " << getpid() << ", [parent] pid: " <<  getppid() << std::endl;
            // std::cout << "New Server with ID: " << s.second->get_server_id() << std::endl;
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

