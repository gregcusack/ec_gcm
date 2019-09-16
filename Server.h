//
// Created by greg on 9/16/19.
//

#ifndef EC_GCM_SERVER_H
#define EC_GCM_SERVER_H

#include <iostream>
#include <cstdint>
#include <thread>
#include "types/msg.h"
#include "types/k_msg.h"
#include "Manager.h"
#include "om.h"


#define __MAX_CLIENT__ 30
#define __BUFFSIZE__ 16
#define __FAILED__ -1
#define __QUOTA__ 50000

namespace ec {
//    struct Manager;
    class Server {
    public:
        explicit Server(ip4_addr _ip_address, uint16_t _port);
        void initialize_server();

        void set_manager(Manager *man) { m = man; }

        struct server_t {
            int32_t sock_fd;
            struct sockaddr_in addr;
        };


        Manager *manager() { return m; }


        void serve();

        void handle_client_reqs(void *clifd);

        static void *handle_client_helper(void *clifd);

        int64_t handle_req(char *buffer);

        int64_t handle_cpu_req(msg_t *req);

        int64_t handle_mem_req(msg_t *req);



        ip4_addr get_ip() { return ip_address; }
        uint16_t get_port() { return port; }
        int32_t get_test_var() { return test; }


    private:
        ip4_addr ip_address;
        uint16_t port;
        struct server_t server_socket;
        Manager *m;
        int32_t test;
        uint32_t mem_reqs;
        uint64_t cpu_limit;     //TODO: delete this. idk what this is supposed to be
        uint64_t memory_limit;
        bool server_initialized;
    };
}


#endif //EC_GCM_SERVER_H
