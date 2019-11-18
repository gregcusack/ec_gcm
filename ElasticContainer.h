//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_EC_MANAGER_H
#define EC_GCM_EC_MANAGER_H

#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <vector>
#include <string>

#include <chrono>
#include <thread>
#include <fstream>

#include "types/msg.h"
#include "types/k_msg.h"
#include "SubContainer.h"
#include "Agents/AgentClient.h"
#include "om.h"
#include "stats/global/mem_g.h"
#include "stats/global/cpu_g.h"
//#include "Server.h"

#define __ALLOC_FAILED__ -2
#define __ALLOC_SUCCESS__ 1
#define __BUFF_SIZE__ 1024
#define _CPU_ 0
#define _MEM_ 1
#define _INIT_ 2
#define _SLICE_ 3

namespace ec {
    /*!
     * @class manager
     * @brief handles all connections and allocations for a single elastic container
     */
//    struct Server;
    class ElasticContainer {
    using subcontainer_map = std::unordered_map<SubContainer::ContainerId, SubContainer *>;
    public:
        explicit ElasticContainer(uint32_t _ec_id);
        ElasticContainer(uint32_t _ec_id, std::vector<AgentClient*> &_agent_clients);
        ~ElasticContainer();

        /**
         *******************************************************
         * GETTERS
         *******************************************************
         **/
        //MISC
        uint32_t get_ec_id() { return ec_id; }
        const subcontainer_map &get_subcontainers() {return subcontainers;}
        const SubContainer &get_subcontainer(SubContainer::ContainerId &container_id);

        //CPU
        uint64_t get_rt_remaining() { return _cpu.get_runtime_remaining(); }

        //MEM
        uint64_t get_memory_available() { return _mem.get_mem_available(); }
        uint64_t get_memory_slice() { return _mem.get_slice_size(); }

        //AGENTS
        uint32_t get_num_agent_clients() { return agent_clients.size(); }
        const std::vector<AgentClient *> &get_agent_clients() const { return agent_clients; };

        /**
         *******************************************************
         * SETTERS
         *******************************************************
         **/

        //CPU
        void set_ec_period(int64_t _period)  { _cpu.set_period(_period); }   //will need to update maanger too
        void set_quota(int64_t _quota) { _cpu.set_quota(_quota); }
        void set_slice_size(uint64_t _slice_size) { _cpu.set_slice_size(_slice_size); }
        uint64_t refill_runtime();

        //MEM
        void ec_resize_memory_max(int64_t _max_mem) { _mem.set_mem_limit(_max_mem); }
        void ec_decrement_memory_available(uint64_t _mem_to_reduce) { _mem.decr_memory_available(_mem_to_reduce); }
        int64_t ec_set_memory_available(uint64_t mem) { return _mem.set_memory_available(mem); }

        /**
         *******************************************************
         * HELPERS
         *******************************************************
         **/
        //MISC
        SubContainer* create_new_sc(uint32_t cgroup_id, uint32_t host_ip, int sockfd);
        int insert_sc(SubContainer &_sc);

        //CPU

        //MEM


    private:
        uint32_t ec_id;
        subcontainer_map subcontainers;

        //agents
        //TODO: this may need to be a map
        //Passed by reference from ECAPI but owned by GCM
        std::vector<AgentClient *> agent_clients;

        //cpu
//        uint64_t runtime_remaining;
        //TODO: need file/struct of macros - like slice, failed, etc
        //mem
        std::ofstream test_file;

        //test
        int flag;

        global::stats::mem _mem;
        global::stats::cpu _cpu;
    };


}

#endif //EC_GCM_EC_MANAGER_H
