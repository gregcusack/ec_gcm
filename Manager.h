//
// Created by Greg Cusack on 11/5/19.
//

#ifndef EC_GCM_MANAGER_H
#define EC_GCM_MANAGER_H

#include "ECAPI.h"
#include "Agent.h"
#include <cstdint>

namespace ec {
    class Manager : public ECAPI {
    public:
        Manager(uint32_t _ec_id, std::vector<Agent *> &_agents) : ECAPI(_ec_id, _agents){};
        int handle_bandwidth(const msg_t *req, msg_t *res) override;

        int handle_mem_req(const msg_t *req, msg_t *res, int clifd) override;
        uint64_t handle_reclaim_memory(int client_fd) override;

        struct reclaim_msg {
            uint16_t cgroup_id;
            uint32_t is_mem;
            //...maybe it needs more things
        };




    };
}


#endif //EC_GCM_MANAGER_H
