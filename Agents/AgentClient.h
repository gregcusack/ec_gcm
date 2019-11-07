//
// Created by Greg Cusack on 11/6/19.
//

#ifndef EC_GCM_AGENTCLIENT_H
#define EC_GCM_AGENTCLIENT_H

#include "Agent.h"

namespace ec {
    class AgentClient {
    public:
        AgentClient(const Agent* _agent, int _sockfd);

        [[nodiscard]] int get_socket() const { return sockfd_new; }
        [[nodiscard]] om::net::ip4_addr get_agent_ip() const {return agent->get_ip(); }
        [[nodiscard]] uint16_t get_agent_port() const { return agent->get_port(); }


    private:
        const Agent* agent;
        int sockfd_new;

    };
}



#endif //EC_GCM_AGENTCLIENT_H
