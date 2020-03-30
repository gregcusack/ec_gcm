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

#define __ALLOC_FAILED__ 0
#define __ALLOC_SUCCESS__ 1
#define __ALLOC_INIT__ 2
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
    using subcontainer_agent_map = std::unordered_map<SubContainer::ContainerId, AgentClient*>;
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
        AgentClient *get_corres_agent(const SubContainer::ContainerId &container_id){return sc_agent_map[container_id];}
        SubContainer *get_sc_for_update(SubContainer::ContainerId &container_id);

        //CPU
        uint64_t get_cpu_rt_remaining() { return _cpu.get_runtime_remaining(); }
        uint64_t get_cpu_unallocated_rt() { return _cpu.get_unallocated_rt(); }
        uint64_t get_cpu_slice() { return _cpu.get_slice(); }
        uint64_t get_fair_cpu_share() { return fair_cpu_share; }
        uint64_t get_overrun() { return _cpu.get_overrun(); }
        uint64_t get_total_cpu() { return _cpu.get_total_cpu(); }

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

        //MISC
        void add_to_agent_map(SubContainer::ContainerId &id, AgentClient* client) { sc_agent_map.insert({id, client}); }

        //CPU
        void set_ec_period(int64_t _period)  { _cpu.set_period(_period); }   //will need to update maanger too
        void set_quota(int64_t _quota) { _cpu.set_quota(_quota); }
        void set_slice_size(uint64_t _slice_size) { _cpu.set_slice_size(_slice_size); }
        uint64_t refill_runtime();
        void incr_unallocated_rt(uint64_t _incr) { _cpu.incr_unalloacted_rt(_incr); }
        void decr_unallocated_rt(uint64_t _decr) { _cpu.decr_unallocated_rt(_decr); }
        void set_unallocated_rt(uint64_t _val) { _cpu.set_unallocated_rt(_val); }
        void update_fair_cpu_share();
        void incr_total_cpu(uint64_t _incr) { _cpu.incr_total_cpu(_incr); }
        void decr_total_cpu(uint64_t _decr) { _cpu.decr_total_cpu(_decr); }
        void incr_overrun(uint64_t _incr) { _cpu.incr_overrun(_incr); }
        void decr_overrun(uint64_t _decr) { _cpu.decr_overrun(_decr); }
        void set_overrun(uint64_t _val) { _cpu.set_overrun(_val); }

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
        SubContainer* create_new_sc(uint32_t cgroup_id, uint32_t host_ip, int sockfd, uint64_t quota, uint32_t nr_throttled);
        int insert_sc(SubContainer &_sc);

        // int insert_pid(int pid);
        // std::vector<int> get_pids();

        //CPU

        //MEM

    private:
        uint32_t ec_id;
        subcontainer_map subcontainers;
        subcontainer_agent_map sc_agent_map;
        uint64_t fair_cpu_share;

        //Passed by reference from ECAPI but owned by GCM
        std::vector<AgentClient *> agent_clients;


        global::stats::mem _mem;
        global::stats::cpu _cpu;
    };


}

#endif //EC_GCM_EC_MANAGER_H
