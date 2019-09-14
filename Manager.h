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

#define __MAX_CLIENT__ 30
#define __BUFFSIZE__ 16
#define __FAILED__ -1
#define __QUOTA__ 50000

class container;

namespace ec {
    /*!
     * @class manager
     * @brief handles all connections and allocations for a single elastic container
     */
    class Manager {
    using container_map = std::unordered_map<SubContainer::ContainerId, SubContainer *>;
    public:
        Manager(uint32_t _ec_id, ip4_addr ip_address, uint16_t port);

        uint32_t accept_container();       //probably want to return a FD for that new SubContainer thread
        void allocate_container(uint32_t cgroup_id, uint32_t server_ip);
        void allocate_container(uint32_t cgroup_id, std::string server_ip);

//        void serve(Manager *m);

        uint32_t handle(uint32_t cgroup_id, uint32_t server_ip);
//        void set_server(Server *_server) {server = _server;};

        class Server {
        public:
                explicit Server(Manager *_m);
                Manager* manager() { return m; }
                int32_t get_test_var() { return test; }
                void serve();
                void *handle_client_reqs(void* clifd);
                static void *handle_client_helper(void* clifd);
                int64_t handle_req(char* buffer);
                int64_t handle_cpu_req(msg_t *req);
                int64_t handle_mem_req(msg_t *req);

                struct server_t {
                    int32_t sock_fd;
                    struct sockaddr_in addr;
                };
        private:
            struct server_t server_socket;
            Manager *m;
            int32_t test;
            uint32_t mem_reqs;
            uint64_t cpu_limit;     //TODO: delete this. idk what this is supposed to be
            uint64_t memory_limit;
        };


        //getters
        uint32_t get_ec_id() { return ec_id; }
        ip4_addr get_ip() { return ip_address; }
        uint16_t get_port() { return port; }
        const container_map& get_containers() {return containers;}
        SubContainer* get_container(SubContainer::ContainerId &container_id);
        Server& get_server() { return *server; }



    private:
        uint32_t ec_id;
        ip4_addr ip_address;
        uint16_t port;
//        Server *server;
        container_map containers;
        Server *server;

    };


}


#endif //EC_GCM_EC_MANAGER_H
