//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_GLOBALCLOUDMANAGER_H
#define EC_GCM_GLOBALCLOUDMANAGER_H

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include "ElasticContainer.h"
#include "Agent.h"
#include "Manager.h"
#include "om.h"

//std::mutex eclock;

namespace ec {
    class GlobalCloudManager {
        using agents_ip_list = std::vector<std::string>;
        using ec_map = std::unordered_map<uint32_t, ec::ElasticContainer*>;
    public:
//        GlobalCloudManager();
        GlobalCloudManager(std::string ip_addr, uint16_t port, agents_ip_list &agents, std::vector<uint16_t> &ec_ports);
        ~GlobalCloudManager();

        uint32_t create_ec();

        const ec_map& get_ecs() {return ecs;}
        ElasticContainer* get_ec(uint32_t ec_id);


    private:
        ip4_addr                gcm_ip;
        uint16_t                gcm_port;           //unknown if needed

        ec_map                  ecs;
        uint32_t                ec_counter;

        std::vector<Agent*>     agents;
        std::vector<uint16_t>   ec_ports;





    };

}


#endif //EC_GCM_GLOBALCLOUDMANAGER_H
