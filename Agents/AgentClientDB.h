//
// Created by maaz on 2/27/20.
//

#ifndef EC_GCM_AGENTCLIENTDB_H
#define EC_GCM_AGENTCLIENTDB_H



#include <iostream>
#include <unordered_map>
#include "../om.h"
#include "AgentClient.h"

namespace ec{
    class AgentClientDB {
    private:

        std::unordered_map<om::net::ip4_addr , AgentClient*> agents_db;

        static AgentClientDB* agent_clients_db_instance;

        AgentClientDB() {
            agents_db = std::unordered_map<om::net::ip4_addr, AgentClient*>();
        }

    public:

        static AgentClientDB* get_agent_client_db_instance();

        void add_agent_client(AgentClient* new_agent_client);

        void remove_agent_client(const AgentClient& target_agent_client);

        const AgentClient* get_agent_client_by_ip(const om::net::ip4_addr& req) {return agents_db.find(req) != agents_db.end() ? agents_db[req] : NULL;}

        uint32_t get_agent_clients_db_size() {return agents_db.size();}

    };
}



#endif //EC_GCM_AGENTCLIENTDB_H