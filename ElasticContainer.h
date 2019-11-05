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
#include "Agent.h"
#include "om.h"
//#include "Server.h"

#define __ALLOC_FAILED__ -2
#define __ALLOC_SUCCESS__ 1
#define __BUFF_SIZE__ 1024
#define _CPU_ 0
#define _MEM_ 1
#define _INIT_ 2
#define _SLICE_ 3


class container;

namespace ec {
    /*!
     * @class manager
     * @brief handles all connections and allocations for a single elastic container
     */
//    struct Server;
    class ElasticContainer {
    using container_map = std::unordered_map<SubContainer::ContainerId, SubContainer *>;
    public:
        explicit ElasticContainer(uint32_t _ec_id);
        ElasticContainer(uint32_t _ec_id, std::vector<Agent*> &_agents);
        ~ElasticContainer();

        struct memory {
            memory(int64_t _max_mem) : memory_limit(_max_mem) {};
            memory() = default;
            //init to 30000 pages (that's what maziyar has it as)
            uint64_t memory_limit           =   30000;                   //-1 no limit, (in mbytes or pages tbd)
            uint64_t slice_size             =   5000; //pages
            friend std::ostream& operator<<(std::ostream& os, const memory& rhs) {
                os << "memory_limit: " << rhs.memory_limit;
                return os;
            }
        };

        struct cpu {
            cpu(uint64_t _period, int64_t _quota, uint64_t _slice_size)
                    : period(_period), quota(_quota), slice_size(_slice_size) {};
            cpu() = default;
            uint64_t period             =   100000000;      //100 ms
            int64_t quota               =   period;             //-1: no limit, in ms
            uint64_t slice_size         =   5000000;       //5 ms
            friend std::ostream& operator<<(std::ostream& os, const cpu& rhs) {
                os  << "period: " << rhs.period << ", "
                    << "quota: " << rhs.quota << ", "
                    << "slice_size: " << rhs.slice_size;
                return os;
            }
        };

        struct reclaim_msg {
            uint16_t cgroup_id;
            uint32_t is_mem;
            //...maybe it needs more things
        };

        uint32_t accept_container();       //probably want to return a FD for that new SubContainer thread
        void allocate_container(uint32_t cgroup_id, uint32_t server_ip);
        void allocate_container(uint32_t cgroup_id, const std::string &server_ip);
        SubContainer* create_new_sc(uint32_t cgroup_id, uint32_t host_ip, int sockfd);

        uint32_t handle(uint32_t cgroup_id, uint32_t server_ip);

        //getters
        uint32_t get_ec_id() { return ec_id; }
        const container_map& get_containers() {return containers;}
        SubContainer* get_container(SubContainer::ContainerId &container_id);

        int insert_sc(SubContainer &_sc);

        //cpu
        uint64_t get_rt_remaining() { return runtime_remaining; }
        inline uint64_t decr_rt_remaining(uint64_t slice);
        inline uint64_t incr_rt_remaining(uint64_t give_back);
        void reset_rt_remaining() { runtime_remaining = quota; }
        int add_cgroup_to_ec(msg_t *res, const uint32_t cgroup_id, const uint32_t ip, int fd);
        int handle_bandwidth(const msg_t *req, msg_t *res);
        int handle_mem_req(const msg_t *req, msg_t *res, int clifd);
        int handle_slice_req(const msg_t *req, msg_t *res, int clifd);

        uint64_t reclaim_memory(int client_fd);

        uint64_t refill_runtime();

        //agents
        uint32_t get_num_agents() { return agents.size(); }
        const std::vector<Agent*> &get_agents() const;

        void set_max_mem(int64_t _max_mem) { _mem.memory_limit = _max_mem; }
        void set_period(int64_t _period)  { _cpu.period = _period; }   //will need to update maanger too
        void set_quota(int64_t _quota) { _cpu.quota = _quota; }
        void set_slice_size(uint64_t _slice_size) { _cpu.slice_size = _slice_size; }

    private:
        uint32_t ec_id;
        container_map containers;

        //agents
        //TODO: this may need to be a map
        //Passed by reference from Manager but owned by GCM
        std::vector<Agent *> agents;

        //cpu
        uint64_t runtime_remaining;
        int64_t quota;
        //TODO: need file/struct of macros - like slice, failed, etc
        uint64_t slice;
        //mem
        uint64_t memory_available;
        uint64_t mem_slice;
        std::ofstream test_file;

        //test
        int flag;

        memory _mem;
        cpu _cpu;

    };


}

namespace std {
    template<>
    struct hash<ec::Agent> {
        std::size_t operator()(ec::Agent const& p) const {
            auto h1 = std::hash<om::net::ip4_addr>()(p.get_ip());
            auto h2 = std::hash<uint16_t>()(p.get_port());
            auto h3 = std::hash<int>()(p.get_sockfd());
            return h1 xor h2 xor h3;
        }
    };
}

#endif //EC_GCM_EC_MANAGER_H
