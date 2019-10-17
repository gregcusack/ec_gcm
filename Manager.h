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

#include "types/msg.h"
#include "types/k_msg.h"
#include "SubContainer.h"
#include "om.h"
//#include "Server.h"

#define __ALLOC_FAILED__ -2
#define __ALLOC_SUCCESS__ 1
#define __BUFF_SIZE__ 1024
#define _CPU_ 0
#define _MEM_ 1
#define _INIT_ 2


class container;

namespace ec {
    /*!
     * @class manager
     * @brief handles all connections and allocations for a single elastic container
     */
    struct Server;
    class Manager {
    using container_map = std::unordered_map<SubContainer::ContainerId, SubContainer *>;
    public:
        explicit Manager(uint32_t _ec_id);
        Manager(uint32_t _ec_id, int64_t _quota, uint64_t _slice_size,
                uint64_t _mem_limit, uint64_t _mem_slice_size);

        //agents
        struct agent {
            agent()               = default;
            om::net::ip4_addr ip    = om::net::ip4_addr::from_string("127.0.0.1");
            uint16_t port           = 4445;
            int sockfd              = 0;
            friend std::ostream& operator<<(std::ostream& os, const agent& rhs) {
                os << "ip: " << rhs.ip << ", port: " << rhs.port << std::endl;
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

        uint32_t handle(uint32_t cgroup_id, uint32_t server_ip);

        //getters
        uint32_t get_ec_id() { return ec_id; }
        const container_map& get_containers() {return containers;}
        SubContainer* get_container(SubContainer::ContainerId &container_id);

        void set_server(Server *serv) { s = serv; }
        Server *get_server() { return s; }

        int insert_sc(SubContainer &_sc);

        //cpu
        uint64_t get_rt_remaining() { return runtime_remaining; }
        inline uint64_t decr_rt_remaining(uint64_t slice);
        inline uint64_t incr_rt_remaining(uint64_t give_back);
        void reset_rt_remaining() { runtime_remaining = quota; }
        int handle_add_cgroup_to_ec(msg_t *res, uint32_t cgroup_id, uint32_t ip, int fd);
        int handle_bandwidth(const msg_t *req, msg_t *res);
        int handle_mem_req(const msg_t *req, msg_t *res, int clifd);

        uint64_t reclaim_memory(int client_fd);

        uint64_t refill_runtime();

        //agents
        uint32_t get_num_agents() { return agents.size(); }
        const std::vector<agent*> &get_agents() const;
        int alloc_agents(uint32_t num_agents);



    private:
        uint32_t ec_id;
        container_map containers;
        Server *s;

        //agents
        //TODO: this may need to be a map
        std::vector<agent*> agents;

        //cpu
        uint64_t runtime_remaining;
        int64_t quota;
        //TODO: need file/struct of macros - like slice, failed, etc
        uint64_t slice;
        //mem
        uint64_t memory_available;
        uint64_t mem_slice;

        //test
        int flag;

        //cpu
//        uint64_t runtime_remaining;


    };


}

namespace std {
    template<>
    struct hash<ec::Manager::agent> {
        std::size_t operator()(ec::Manager::agent const& p) const {
            auto h1 = std::hash<om::net::ip4_addr>()(p.ip);
            auto h2 = std::hash<uint16_t>()(p.port);
            auto h3 = std::hash<int>()(p.sockfd);
            return h1 xor h2 xor h3;
        }
    };
}

#endif //EC_GCM_EC_MANAGER_H
