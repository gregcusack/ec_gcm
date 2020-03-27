//
// Created by maaz on 2/27/20.
//

#include "AgentClientDB.h"

ec::AgentClientDB * ec::AgentClientDB::get_agent_client_db_instance() {
    if(!agent_clients_db_instance)
        agent_clients_db_instance = new AgentClientDB();

    return agent_clients_db_instance;
}

void ec::AgentClientDB::add_agent_client(AgentClient* new_agent_client) {
    if(agents_db.find(new_agent_client->get_agent_ip()) != agents_db.end()) {
        std::cout << "[Error] This agent has been already added! it cannot be added again." << std::endl;
        return;
    }
    std::cerr << "[dbg] this is ne agent clients added to the db: " << new_agent_client->get_agent_ip() << std::endl;
    agents_db.insert({new_agent_client->get_agent_ip(), new_agent_client});
    std::cerr << "[dbg] add_agent_client: Size of the agent client db: " << agents_db.size() << std::endl;
}

void ec::AgentClientDB::remove_agent_client(const AgentClient& target_agent_client) {
    agents_db.erase(target_agent_client.get_agent_ip());
}
