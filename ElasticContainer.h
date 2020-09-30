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
#include "cAdvisorSDK/include/cAdvisorFacade.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#define __ALLOC_FAILED__ 0
#define __ALLOC_SUCCESS__ 1
#define __ALLOC_INIT__ 2
#define __ALLOC_MEM_FAILED__ 3
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
    using subcontainer_agentclient_map = std::unordered_map<SubContainer::ContainerId, AgentClient*>;
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
        const int get_num_subcontainers() { return subcontainers.size(); }
        subcontainer_map *get_subcontainers_map_for_update() { return &subcontainers; }
        SubContainer &get_subcontainer(const SubContainer::ContainerId &container_id);
        AgentClient* get_corres_agent(const SubContainer::ContainerId &container_id){return sc_ac_map[container_id];}
        SubContainer *get_sc_for_update(SubContainer::ContainerId &container_id);
        const subcontainer_agentclient_map &get_sc_ac_map() {return sc_ac_map;}
        subcontainer_agentclient_map *get_sc_ac_map_for_update() {return &sc_ac_map; }
        void get_sc_from_agent(const AgentClient* client, std::vector<SubContainer::ContainerId> &res);

        //CPU
        uint64_t get_cpu_unallocated_rt() { return _cpu.get_unallocated_rt(); }
        uint64_t get_cpu_slice() { return _cpu.get_slice(); }
        uint64_t get_fair_cpu_share() { return fair_cpu_share; }
        uint64_t get_overrun() { return _cpu.get_overrun(); }
        uint64_t get_total_cpu() { return _cpu.get_total_cpu(); }
        uint64_t get_alloc_rt() { return _cpu.get_alloc_rt(); }


        //MEM
        uint64_t get_unallocated_memory_in_pages() { return _mem.get_unallocated_memory_in_pages(); }
        uint64_t get_allocated_memory_in_pages() { return _mem.get_allocated_memory_in_pages(); }
        uint64_t get_memory_slice() { return _mem.get_slice_size(); }
        uint64_t get_mem_limit_in_pages() { return _mem.get_mem_limit_in_pages(); }

        uint64_t get_sc_memory_limit_in_bytes(const ec::SubContainer::ContainerId &sc_id);
        uint64_t get_tot_mem_alloc_in_pages();

        /**
         *******************************************************
         * SETTERS
         *******************************************************
         **/

        //MISC
        void add_to_sc_ac_map(SubContainer::ContainerId &id, AgentClient* client) { sc_ac_map.insert({id, client}); }

        //CPU
        void set_slice_size(uint64_t _slice_size) { _cpu.set_slice_size(_slice_size); }
        void incr_unallocated_rt(uint64_t _incr) { _cpu.incr_unalloacted_rt(_incr); }
        void decr_unallocated_rt(uint64_t _decr) { _cpu.decr_unallocated_rt(_decr); }
        void set_unallocated_rt(uint64_t _val) { _cpu.set_unallocated_rt(_val); }
        void update_fair_cpu_share();
        void incr_total_cpu(uint64_t _incr) { _cpu.incr_total_cpu(_incr); }
        void decr_total_cpu(uint64_t _decr) { _cpu.decr_total_cpu(_decr); }
        void incr_overrun(uint64_t _incr) { _cpu.incr_overrun(_incr); }
        void decr_overrun(uint64_t _decr) { _cpu.decr_overrun(_decr); }
        void set_overrun(uint64_t _val) { _cpu.set_overrun(_val); }
        void set_total_cpu(uint64_t _val) { _cpu.set_total_cpu(_val); }
        void incr_alloc_rt(uint64_t _incr) { _cpu.incr_alloc_rt(_incr); }
        void decr_alloc_rt(uint64_t _decr) { _cpu.decr_alloc_rt(_decr); }


        //MEM
        void set_memory_limit_in_pages(int64_t _max_mem) { _mem.set_memory_limit_in_pages(_max_mem); }
        void incr_memory_limit_in_pages(uint64_t _incr) { _mem.incr_memory_limit_in_pages(_incr); }
        void decr_memory_limit_in_pages(uint64_t _decr) { _mem.decr_memory_limit_in_pages(_decr); }

        int64_t set_unalloc_memory_in_pages(uint64_t mem) { return _mem.set_unallocated_memory_in_pages(mem); }
        void decr_unalloc_memory_in_pages(uint64_t _mem_to_reduce) {
            _mem.decr_unallocated_memory_in_pages(_mem_to_reduce); }
        void incr_unalloc_memory_in_pages(uint64_t _mem_to_incr) {
            _mem.incr_unallocated_memory_in_pages(_mem_to_incr); }

        void set_alloc_memory_in_pages(int64_t _ma) { _mem.set_allocated_memory_in_pages(_ma); }
        void incr_alloc_memory_in_pages(int64_t _incr) { _mem.incr_allocated_memory_in_pages(_incr); }
        void decr_alloc_memory_in_pages(int64_t _decr) { _mem.decr_allocated_memory_in_pages(_decr); }



        /**
         *******************************************************
         * HELPERS
         *******************************************************
         **/
        //MISC
        SubContainer* create_new_sc(uint32_t cgroup_id, uint32_t host_ip, int sockfd);
        SubContainer* create_new_sc(uint32_t cgroup_id, uint32_t host_ip, int sockfd, uint64_t quota, uint32_t nr_throttled);
        int insert_sc(SubContainer &_sc);

        int ec_delete_from_subcontainers_map(const SubContainer::ContainerId &sc_id);


    private:
        uint32_t ec_id;
        subcontainer_map subcontainers;
        subcontainer_agentclient_map sc_ac_map;
        uint64_t fair_cpu_share;
        std::mutex sc_lock;

        //test
        int flag{};
        std::ofstream test_file;

        global::stats::mem _mem;
        global::stats::cpu _cpu;
    };


}

#endif //EC_GCM_EC_MANAGER_H
