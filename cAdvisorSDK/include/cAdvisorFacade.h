#ifndef CADVISOR_FACADE_H
#define CADVISOR_FACADE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include "../../jsonSDK/include/JSONFacade.h"
#include "../../protoBufSDK/include/ProtoBufFacade.h"
#include "../../protoBufSDK/msg.pb.h"

namespace ec {
    namespace Facade {
        namespace MonitorFacade {
            class CAdvisor {
                public:
                    static uint64_t getContCPULimit(const std::string &agent_ip, const std::string &docker_container_id);
                    static uint64_t getContCPUQuota(const std::string &agent_ip, const std::string &docker_container_id);
                    static uint64_t getContCPUShares(const std::string &agent_ip, const std::string &docker_container_id);
                    static uint64_t getContMemLimit(const std::string &agent_ip, const std::string &docker_container_id);
                    static uint64_t getContMemUsage(const std::string &agent_ip, const std::string &docker_container_id);
            };
        }
    }
}

#endif //CADVISOR_FACADE_H
