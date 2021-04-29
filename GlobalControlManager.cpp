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

ec::GlobalControlManager::GlobalControlManager(std::string ip_addr, uint16_t port, agents_ip_list &_agent_ips, std::vector<ports_t> &_server_ports)
    : gcm_ip(ec::ip4_addr::from_string(std::move(ip_addr))), mngr(nullptr), gcm_port(port), controller_ports(_server_ports), manager_counts(1) {

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

    auto ports = controller_ports[manager_counts - 1];
    mngr = new Manager(manager_counts, gcm_ip, ports, agents);

//    uint16_t server_port = server_ports[manager_counts - 1]; //Give new EC, the next available port in the list
//    mngr = new Manager(manager_counts, gcm_ip, server_port, agents);

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

bool ec::GlobalControlManager::init_agent_connections() {
    int sockfd, i;
    struct sockaddr_in servaddr;
    uint32_t num_connections = 0;

    AgentClientDB* agent_clients_db = AgentClientDB::get_agent_client_db_instance();
    for(const auto &ag : agents) {
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            SPDLOG_CRITICAL("GCM Socket creation failed. Agent is not up!");
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(ag->get_port());
        servaddr.sin_addr.s_addr = inet_addr((ag->get_ip()).to_string().c_str());
        SPDLOG_DEBUG("ag->ip: {}", ag->get_ip());

        if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            SPDLOG_ERROR("[ERROR] GCM: Connection to agent_clients failed. Agent on ip: {} is not connected", ag->get_ip());
            SPDLOG_ERROR("Are the agents up?");
        }
        else {
            num_connections++;
        }
        auto* ac = new AgentClient(ag, sockfd);
        agent_clients_db->add_agent_client(ac);

        SPDLOG_TRACE("[dbg] Agent client added to db and agent_clients sockfd: {}, {} agent db size is: {}", sockfd, \
        agent_clients_db->get_agent_client_by_ip(ag->get_ip())->get_socket(), agent_clients_db->get_agent_clients_db_size());

    }
    return num_connections == agent_clients_db->get_agent_clients_db_size();

}

