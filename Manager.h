//
// Created by Greg Cusack on 11/5/19.
//

#ifndef EC_GCM_MANAGER_H
#define EC_GCM_MANAGER_H

#include "ECAPI.h"
#include "Server.h"
//#include "Agents/AgentClient.h"
#include <cstdint>
#include <mutex>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

namespace ec {
    class Manager : public ECAPI, public Server {
    public:
        //TODO: initialize ECAPI and SERVER here?
        Manager(uint32_t server_counts, ip4_addr gcm_ip, uint16_t server_port, std::vector<Agent *> &agents);
//        Manager(uint32_t _ec_id, std::vector<AgentClient *> &_agent_clients) : ECAPI(_ec_id, _agent_clients) {};

        int handle_cpu_usage_report(const msg_t *req, msg_t *res) override;

        int handle_mem_req(const msg_t *req, msg_t *res, int clifd) override;
        uint64_t handle_reclaim_memory(int client_fd) override;

        int handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd);
        void start(const std::string &app_name, const std::vector<std::string> &app_images, const std::vector<std::string> &pod_names, const std::string &gcm_ip);
        virtual void run();

    private:
        std::mutex cpulock;
        std::mutex memlock;

    };
}


#endif //EC_GCM_MANAGER_H
