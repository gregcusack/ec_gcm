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
#include "spdlog/spdlog.h"
#include <grpc++/grpc++.h>
#include "containerUpdateGrpc.pb.h"
#include "containerUpdateGrpc.grpc.pb.h"
#include <exception>


using namespace google::protobuf::io;


#define __BUFFSIZE__ 1024

namespace ec {
    namespace rpc {
        class AgentClient {
        public:
            AgentClient(const Agent *_agent);

            int connectAgentGrpc();

            [[nodiscard]] om::net::ip4_addr get_agent_ip() const { return agent->get_ip(); }

            [[nodiscard]] uint16_t get_agent_port() const { return agent->get_port(); }

            int updateContainerQuota(uint32_t cgroup_id, uint64_t new_quota, const std::string& change, uint32_t seq_num);

            int64_t resizeMemoryLimitPages(uint32_t cgroup_id, uint64_t new_mem_limit);

            int64_t getMemoryUsageBytes(uint32_t cgroup_id);
            int64_t getMemoryLimitBytes(uint32_t cgroup_id);


        private:
            const Agent *agent;
//            int sockfd_new;
            std::mutex sendlock;




            std::unique_ptr<ec::rpc::containerUpdate::ContainerUpdateHandler::Stub> stub_;
            std::shared_ptr<grpc_impl::Channel> channel_;

        };
    }
}

namespace std {
    template<>
    struct hash<ec::rpc::AgentClient> {
        std::size_t operator()(ec::rpc::AgentClient const& p) const {
            auto h1 = std::hash<om::net::ip4_addr>()(p.get_agent_ip());
            auto h2 = std::hash<uint16_t>()(p.get_agent_port());
            return h1 xor h2;
        }
    };
}



#endif //EC_GCM_AGENTCLIENT_H
