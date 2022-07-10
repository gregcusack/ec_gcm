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
#include "types/ports.h"


#define __NUM_THREADS__ 32
//std::mutex eclock;

namespace ec {
    class GlobalControlManager {
        using agents_ip_list = std::vector<std::string>;
        using manager_map = std::unordered_map<int, ec::Manager*>;
    public:
//        GlobalControlManager();
        GlobalControlManager(std::string ip_addr, uint16_t port, agents_ip_list &agents, std::vector<uint16_t> &_server_ports);
        GlobalControlManager(std::string ip_addr, uint16_t port, agents_ip_list &agents, std::vector<ports_t> &_server_ports);
        ~GlobalControlManager();

        void run(const std::string &app_name, const std::string &_gcm_ip, const int num_containers);

        uint32_t create_manager();

        const manager_map& get_managers() {return managers;}
        const Manager& get_manager(int manager_id) const;
        std::thread* get_thread() {return &grpc_thread;}

        struct app_thread_args {
            app_thread_args()              = default;
            std::string app_name           = "";
            std::vector<std::string> *app_images = nullptr;
        };

        bool init_agent_connections();
        void join_grpc_threads();

    private:
        ip4_addr                gcm_ip;
        uint16_t                gcm_port;           //unknown if needed
        std::vector<ports_t>    controller_ports;

        manager_map              managers;
        int                manager_counts;


        std::vector<Agent*>     agents;
        std::vector<uint16_t>   server_ports;

        //API
        Manager* mngr;
//        std::vector<std::thread> grpc_thread_vec;
        std::thread grpc_thread;

//        std::thread thr_quota_, thr_resize_mem_, thr_get_mem_lim_, thr_get_mem_usage_;

    };

}


#endif //EC_GCM_GLOBALCONTROLMANAGER_H
