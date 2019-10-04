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
#include <pthread.h>

#include "types/msg.h"
#include "types/k_msg.h"
#include "SubContainer.h"
#include "om.h"
//#include "Server.h"

#define __ALLOC_FAILED__ -2
#define __ALLOC_SUCCESS__ 1
#define _CPU_ 0
#define _MEM_ 1
#define _INIT_ 2
#define __MEM_SLICE__



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
        Manager(uint32_t _ec_id);
        Manager(uint32_t _ec_id, int64_t _quota, uint64_t _slice_size,
                uint64_t _mem_limit, uint64_t _mem_slice_size);

        uint32_t accept_container();       //probably want to return a FD for that new SubContainer thread
        void allocate_container(uint32_t cgroup_id, uint32_t server_ip);
        void allocate_container(uint32_t cgroup_id, std::string server_ip);

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
        int handle_add_cgroup_to_ec(msg_t *res, uint32_t cgroup_id, uint32_t ip);
        int handle_bandwidth(const msg_t *req, msg_t *res);
        int handle_mem_req(const msg_t *req, msg_t *res);

        uint64_t refill_runtime();





    private:
        uint32_t ec_id;
//        ip4_addr ip_address;
//        uint16_t port;
        container_map containers;
        Server *s;

        //cpu
        uint64_t runtime_remaining;
        int64_t quota;
        //TODO: need file/struct of macros - like slice, failed, etc
        uint64_t slice;
        //mem
        uint64_t memory_available;
        uint64_t mem_slice;

        //cpu
//        uint64_t runtime_remaining;


    };


}


#endif //EC_GCM_EC_MANAGER_H
