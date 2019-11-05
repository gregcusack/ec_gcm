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
#include "Agent.h"
#include "Manager.h"
//#include "ElasticContainer.h"
#include "Manager.h"
#include "om.h"


#define __MAX_CLIENT__ 30
#define __BUFFSIZE__ 1024
#define __FAILED__ -1
//#define __ALLOC_FAILED__ 0
#define __QUOTA__ 50000




namespace ec {
//    struct ElasticContainer;
    class Server {
    public:
        Server(uint32_t server_id, ip4_addr _ip_address, uint16_t _port, std::vector<Agent *> &_agents);
        ~Server() = default;
        void initialize_server();

//        void set_ec(ElasticContainer *ec) { ec = ec; }

        struct server_t {
            int32_t sock_fd;
            struct sockaddr_in addr;
        };
        struct serv_thread_args {
            serv_thread_args()              = default;
            int clifd                       = 0;
            struct sockaddr_in *cliaddr     = nullptr;
        };

//        ElasticContainer *manager() { return ec; }

        void serve();

        void handle_client_reqs(void *clifd);

        //TODO: make return values error codes and pass struct via "msg_res"
        int handle_req(const msg_t *req, msg_t *res, serv_thread_args* args);

        int serve_add_cgroup_to_ec(const msg_t *req, msg_t *res, serv_thread_args* args);

        int serve_cpu_req(const msg_t *req, msg_t *res, serv_thread_args* args);

        int serve_mem_req(const msg_t *req, msg_t *res, serv_thread_args* args);

        int serve_acquire_slice(const msg_t *req, msg_t *res, serv_thread_args* args);


        ip4_addr get_ip() { return ip_address; }
        uint16_t get_port() { return port; }
        uint32_t get_server_id() { return server_id; }
        std::mutex mtx;

    private:
        uint32_t server_id;
        ip4_addr ip_address;
        uint16_t port;
        std::vector<Agent *> agents;
        Manager *manager;
        struct server_t server_socket;

        bool server_initialized;
        uint32_t num_of_cli;
    };
}


#endif //EC_GCM_SERVER_H
