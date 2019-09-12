//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_GLOBALCLOUDMANAGER_H
#define EC_GCM_GLOBALCLOUDMANAGER_H

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include "Manager.h"
#include "om.h"

namespace ec {
    class GlobalCloudManager {
        using manager_map = std::unordered_map<uint32_t, ec::Manager*>;
    public:
        GlobalCloudManager();
        GlobalCloudManager(std::string ip_addr, uint16_t port);

        uint32_t create_new_manager();

        const manager_map& get_managers() {return managers;}
        Manager& get_manager(uint32_t ec_id);




    private:
        om::net::ip4_addr   gcm_ip;
        uint16_t            gcm_port;

        manager_map managers;
        uint32_t manager_counter;


    };

}


#endif //EC_GCM_GLOBALCLOUDMANAGER_H
