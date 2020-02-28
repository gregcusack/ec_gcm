#ifndef CADVISOR_FACADE_H
#define CADVISOR_FACADE_H

#include <iostream>
#include <fstream>
#include <sstream>
#include "../../jsonSDK/include/JSONFacade.h"

class CAdvisor {
    public:
        CAdvisor() {}
        // bool checkCAdvisor();
        uint64_t getContCPULimit(const std::string agent_ip, const std::string docker_container_id);
        uint64_t getContMemLimit(const std::string agent_ip, const std::string docker_container_id);

        // std::string exec(const std::string cmd);
    private:
        std::string _cAdvisor_cont_id;
};

#endif //CADVISOR_FACADE_H
