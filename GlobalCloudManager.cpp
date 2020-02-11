//
// Created by greg on 9/12/19.
//

#include "GlobalCloudManager.h"

//_ec::GlobalCloudManager::GlobalCloudManager()
//    : gcm_ip(_ec::ip4_addr::from_string("127.0.0.1")), gcm_port(4444), server_counts(1) {}

ec::GlobalCloudManager::GlobalCloudManager(std::string ip_addr, uint16_t port, agents_ip_list &_agent_ips, std::vector<uint16_t> &_server_ports)
    : gcm_ip(ec::ip4_addr::from_string(std::move(ip_addr))), gcm_port(port), server_ports(_server_ports), server_counts(1) {

    for(const auto &i : _agent_ips) {
        agents.emplace_back(new Agent(i));
    }
    for(auto &i : agents) {
        std::cout << "agent_clients: " << *i << std::endl;
    }

    if(agents.size() != _agent_ips.size()) {
        std::cout << "ERROR: alloc agents failed!" << std::endl;
        exit(EXIT_FAILURE);
    }
}

uint32_t ec::GlobalCloudManager::create_server() {
    if(servers.find(server_counts) != servers.end()) {
        std::cout << "ERROR: Error allocating new Server. Server IDs not correct" << std::endl;
        return 0;
    }

    uint16_t server_port = server_ports[server_counts - 1]; //Give new EC, the next available port in the list
    mngr = new Manager(server_counts, gcm_ip, server_port, agents);

    servers.insert({server_counts, mngr});

//    eclock.lock();
    server_counts++;
//    eclock.unlock();
    return mngr->get_server_id();
}

const ec::Manager &ec::GlobalCloudManager::get_server(const uint32_t server_id) const {
    auto itr = servers.find(server_id);
    if(itr == servers.end()) {
        std::cout << "ERROR: No Server with server_id: " << server_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return *itr->second;
}


ec::GlobalCloudManager::~GlobalCloudManager() {
    for(auto a : agents) {
        delete a;
    }
    agents.clear();
    for(auto &s : servers) {
        delete s.second;
    }
    servers.clear();
}

void ec::GlobalCloudManager::run(std::string app_name, std::vector<std::string> app_images) {

    std::thread threads[32];
    //app_thread_args *args;
    int32_t num_of_cli = 0;

    for(const auto &s : servers) {
        if(fork() == 0) {
            std::cout << "[child] pid: " << getpid() << ", [parent] pid: " <<  getppid() << std::endl;
            std::cout << "server_id: " << s.second->get_server_id() << std::endl;
            
            // Spin up 2 threads here: 1 for the server listening on events
            //s.second->initialize();
            //std::thread t1(&ec::Server::serve, s.second);

            // Another to deploy the application
            //std::thread t2(&ec::Manager::start, s.second, app_name, app_images);

            //s.second->start(app_name, app_images);

            //t2.join();
            //t1.join();

            s.second->start(app_name, app_images);

        }
        else {
            continue;
        }
        break;
    }
    for(const auto &i : servers) {
        wait(nullptr);
    }

}

