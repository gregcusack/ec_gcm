//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_GLOBALCONTROLMANAGER_H
#define EC_GCM_GLOBALCONTROLMANAGER_H

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <sys/types.h>
#include <sys/wait.h>
#include "ECAPI.h"
#include "Server.h"
#include "Agents/Agent.h"
#include "ElasticContainer.h"
#include "om.h"
#include <thread>
#include "Manager.h"
#include "spdlog/spdlog.h"


#define __NUM_THREADS__ 32
//std::mutex eclock;

namespace ec {
    class GlobalControlManager {
        using agents_ip_list = std::vector<std::string>;
        using manager_map = std::unordered_map<int, ec::Manager*>;
    public:
//        GlobalControlManager();
        GlobalControlManager(std::string ip_addr, uint16_t port, agents_ip_list &agents, std::vector<uint16_t> &_server_ports);
        ~GlobalControlManager();

        void run(const std::string &app_name, const std::string &_gcm_ip);

        uint32_t create_manager();

        const manager_map& get_managers() {return managers;}
        const Manager& get_manager(int manager_id) const;

        struct app_thread_args {
            app_thread_args()              = default;
            std::string app_name           = "";
            std::vector<std::string> *app_images = nullptr;
        };

    private:
        ip4_addr                gcm_ip;
        uint16_t                gcm_port;           //unknown if needed

        manager_map              managers;
        int                manager_counts;


        std::vector<Agent*>     agents;
        std::vector<uint16_t>   server_ports;

        //API
        Manager* mngr;

    };

}


#endif //EC_GCM_GLOBALCONTROLMANAGER_H
