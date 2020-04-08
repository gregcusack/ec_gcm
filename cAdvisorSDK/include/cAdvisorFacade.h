#ifndef CADVISOR_FACADE_H
#define CADVISOR_FACADE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include "../../jsonSDK/include/JSONFacade.h"

namespace ec {
    namespace Facade {
        namespace MonitorFacade {
            class CAdvisor {
                public:
                    static uint64_t getContCPUThrottledPeriods(const std::string &agent_ip, const std::string &docker_container_id);
                    static uint64_t getContCPUQuota(const std::string &agent_ip, const std::string &docker_container_id);
                    static uint64_t getContCPUShares(const std::string &agent_ip, const std::string &docker_container_id);
                    static uint64_t getContMemLimit(const std::string &agent_ip, const std::string &docker_container_id);
                    static uint64_t getContMemUsage(const std::string &agent_ip, const std::string &docker_container_id);
            };
        }
    }
}

#endif //CADVISOR_FACADE_H
