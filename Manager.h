//
// Created by Greg Cusack on 11/5/19.
//

#ifndef EC_GCM_MANAGER_H
#define EC_GCM_MANAGER_H

#include "ECAPI.h"
#include "Server.h"
//#include "Agents/AgentClient.h"
#include <cstdint>

namespace ec {
    class Manager : public ECAPI, public Server {
        public:
            Manager(uint32_t &server_counts, ip4_addr &gcm_ip, uint16_t &server_port, std::vector<Agent *> &agents);
            int handle_cpu_req(const msg_t *req, msg_t *res) override;

            int handle_mem_req(const msg_t *req, msg_t *res, int clifd) override;
            uint64_t handle_reclaim_memory(int client_fd) override;

            int handle_req(const msg_t *req, msg_t *res, uint32_t &host_ip, int &clifd);
            void start(const std::string &app_name, const std::vector<std::string> &app_images, const std::string &gcm_ip);
            virtual void run();
        struct reclaim_msg {
            uint16_t cgroup_id;
            uint32_t is_mem;
            //...maybe it needs more things
        };

    };
}


#endif //EC_GCM_MANAGER_H
