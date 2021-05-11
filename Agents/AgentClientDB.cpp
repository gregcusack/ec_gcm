//
// Created by maaz on 2/27/20.
//

#include "AgentClientDB.h"

ec::AgentClientDB * ec::AgentClientDB::get_agent_client_db_instance() {
    if(!agent_clients_db_instance)
        agent_clients_db_instance = new AgentClientDB();

    return agent_clients_db_instance;
}

void ec::AgentClientDB::add_agent_client(rpc::AgentClient* new_agent_client) {
    if(agents_db.find(new_agent_client->get_agent_ip()) != agents_db.end()) {
        SPDLOG_ERROR("This agent has been already added! it cannot be added again.");
        return;
    }
    agents_db.insert({new_agent_client->get_agent_ip(), new_agent_client});
}

void ec::AgentClientDB::remove_agent_client(const rpc::AgentClient& target_agent_client) {
    agents_db.erase(target_agent_client.get_agent_ip());
}

ec::rpc::AgentClient *ec::AgentClientDB::get_agent_client_by_ip(const om::net::ip4_addr &req) {
    auto itr = agents_db.find(req);
    if(itr != agents_db.end()) {
        return itr->second;
    }
    return nullptr;
}

