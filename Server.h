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
#define __BUFFSIZE__ 1024
#define __FAILED__ -1
//#define __ALLOC_FAILED__ 0
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
        struct serv_thread_args {
            serv_thread_args()              = default;
            int clifd                       = 0;
            struct sockaddr_in *cliaddr     = nullptr;
        };


        Manager *manager() { return m; }


        void serve();

        void handle_client_reqs(void *clifd);

        //TODO: make return values error codes and pass struct via "msg_res"
        int handle_req(const msg_t *req, msg_t *res, serv_thread_args* args);

        int serve_add_cgroup_to_ec(const msg_t *req, msg_t *res, serv_thread_args* args);

        int serve_cpu_req(const msg_t *req, msg_t *res, serv_thread_args* args);

        int serve_mem_req(const msg_t *req, msg_t *res, serv_thread_args* args);

        int serve_acquire_slice(const msg_t *req, msg_t *res, serv_thread_args* args);

        int init_agents_connection(int num_agents);


        ip4_addr get_ip() { return ip_address; }
        uint16_t get_port() { return port; }
        int32_t get_test_var() { return test; }
        std::mutex mtx;




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
        uint32_t num_of_cli;
    };
}


#endif //EC_GCM_SERVER_H
