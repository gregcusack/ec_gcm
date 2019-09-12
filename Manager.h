//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_EC_MANAGER_H
#define EC_GCM_EC_MANAGER_H

#include <cstdint>
#include <iostream>
#include <unordered_map>
#include "SubContainer.h"
#include "om.h"

class container;

namespace ec {
    /*!
     * @class manager
     * @brief handles all connections and allocations for a single elastic container
     */
    class Manager {
    public:
        Manager(uint32_t _ec_id);

        uint32_t accept_new_cgroup();       //probably want to return a FD for that new SubContainer thread
        void allocate_new_cgroup(uint32_t cgroup_id, uint32_t server_ip);
        void allocate_new_cgroup(uint32_t cgroup_id, std::string server_ip);

        //cpu
        uint32_t refill_ec_runtime();

        uint32_t allocate_runtime();

        uint32_t takeback_runtime();

        //mem
        uint32_t set_cgroup_mem_max();

        uint32_t takeback_memory();

        uint32_t handle(uint32_t cgroup_id, uint32_t server_ip);

        //getters
        uint32_t get_ec_id() { return ec_id; };


    private:
        uint32_t ec_id;
        std::unordered_map<ec::SubContainer::ContainerId, ec::SubContainer *> containers;


        //cpu
        uint64_t period;
        uint64_t runtime;
        uint64_t runtime_remaining;
        uint64_t runtime_slice_size;
        //high resolution timer

        //mem
        uint64_t total_mem;
        uint64_t mem_remaining;


    };
}


#endif //EC_GCM_EC_MANAGER_H
