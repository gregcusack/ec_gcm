//
// Created by Greg Cusack on 11/6/19.
//

#ifndef EC_GCM_AGENTCLIENT_H
#define EC_GCM_AGENTCLIENT_H

#include "Agent.h"
#include "../types/msg.h"
#include <iostream>
#include <functional> //for std::hash
#include <string>
#include "../protoBufSDK/msg.pb.h"
#include "../protoBufSDK/include/ProtoBufFacade.h"
#include <mutex>
#include <chrono>


using namespace google::protobuf::io;


#define __BUFFSIZE__ 1024

namespace ec {
    class AgentClient {
    public:
        AgentClient(const Agent* _agent, int _sockfd);

        [[nodiscard]] int get_socket() const { return sockfd_new; }
        [[nodiscard]] om::net::ip4_addr get_agent_ip() const {return agent->get_ip(); }
        [[nodiscard]] uint16_t get_agent_port() const { return agent->get_port(); }
        int64_t send_request(const struct msg_struct::ECMessage &msg) const;


    private:
        const Agent* agent;
        int sockfd_new;
        std::mutex sendlock;


    };
}

namespace std {
    template<>
    struct hash<ec::AgentClient> {
        std::size_t operator()(ec::AgentClient const& p) const {
            auto h1 = std::hash<om::net::ip4_addr>()(p.get_agent_ip());
            auto h2 = std::hash<uint16_t>()(p.get_agent_port());
            auto h3 = std::hash<int>()(p.get_socket());
            return h1 xor h2 xor h3;
        }
    };
}



#endif //EC_GCM_AGENTCLIENT_H
