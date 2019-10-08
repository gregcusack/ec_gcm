//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_GLOBALCLOUDMANAGER_H
#define EC_GCM_GLOBALCLOUDMANAGER_H

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include "ElasticContainer.h"
#include "Manager.h"
#include "om.h"

//std::mutex eclock;

namespace ec {
    class GlobalCloudManager {
        using manager_map = std::unordered_map<uint32_t, ec::Manager*>;
        using ec_map = std::unordered_map<uint32_t, ec::ElasticContainer*>;
    public:
        GlobalCloudManager();
        GlobalCloudManager(std::string ip_addr, uint16_t port);

        uint32_t create_ec(uint32_t num_agents);

        const ec_map& get_ecs() {return ecs;}
        ElasticContainer* get_ec(uint32_t ec_id);




    private:
        ip4_addr        gcm_ip;
        uint16_t        gcm_port;           //unknown if needed

        ec_map          ecs;
        uint32_t        ec_counter;


    };

}


#endif //EC_GCM_GLOBALCLOUDMANAGER_H
