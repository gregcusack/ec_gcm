//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_GLOBALCLOUDMANAGER_H
#define EC_GCM_GLOBALCLOUDMANAGER_H

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <sys/types.h>
//#include <wait.h>
#include "ECAPI.h"
#include "Server.h"
#include "Agents/Agent.h"
#include "ElasticContainer.h"
#include "om.h"

//std::mutex eclock;

namespace ec {
    class GlobalCloudManager {
        using agents_ip_list = std::vector<std::string>;
        using server_map = std::unordered_map<uint16_t, ec::Server*>;
    public:
//        GlobalCloudManager();
        GlobalCloudManager(std::string ip_addr, uint16_t port, agents_ip_list &agents, std::vector<uint16_t> &_server_ports);
        ~GlobalCloudManager();

        void run();

        uint32_t create_server();

        const server_map& get_servers() {return servers;}
        const Server& get_server(uint32_t server_id) const;

    private:
        ip4_addr                gcm_ip;
        uint16_t                gcm_port;           //unknown if needed

        server_map              servers;
        uint32_t                server_counts;


        std::vector<Agent*>     agents;
        std::vector<uint16_t>   server_ports;

    };

}


#endif //EC_GCM_GLOBALCLOUDMANAGER_H
