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
        std::cout << "agent: " << *i << std::endl;
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
    auto *server = new Server(server_counts, gcm_ip, server_port, agents);
    servers.insert({server_counts, server});

//    eclock.lock();
    server_counts++;
//    eclock.unlock();
    return server->get_server_id();
}

const ec::Server &ec::GlobalCloudManager::get_server(const uint32_t server_id) const {
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

int ec::GlobalCloudManager::init_agent_connections() {
    int sockfd, i;
    struct sockaddr_in servaddr;
    int num_connections = 0;

//    for(i = 0; i < num_agents; i++) {
    for(const auto &ag : agents) {
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cout << "[ERROR]: GCM Socket creation failed. Agent is not up!" << std::endl;
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(ag->get_port());
        servaddr.sin_addr.s_addr = inet_addr((ag->get_ip()).to_string().c_str());

        if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            std::cout << "[ERROR] GCM: Connection to agent failed. \n Agent on ip: " << ag->get_ip() << "is not connected" << std::endl;
            std::cout << "Are the agents up?" << std::endl;
        }
        else {
            num_connections++;
        }

       ag->set_sockfd(sockfd);
        std::cout << "agent sockfd: " << sockfd << ", " << ag->get_sockfd() << std::endl;
    }
    return num_connections == agents.size();
}

void ec::GlobalCloudManager::run() {
    for(const auto &s : servers) {
        if(fork() == 0) {
            std::cout << "[child] pid: " << getpid() << ", [parent] pid: " <<  getppid() << std::endl;
            std::cout << "server_id: " << s.second->get_server_id() << std::endl;

            s.second->initialize();
            s.second->serve();
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

