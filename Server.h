//
// Created by greg on 9/16/19.
//

#ifndef EC_GCM_SERVER_H
#define EC_GCM_SERVER_H

#include <iostream>
#include <cstdint>
#include <thread>
#include <string>
#include "types/msg.h"
#include "Agents/Agent.h"
#include "Agents/AgentClient.h"
#include "types/ports.h"
#include "om.h"
#include "types/types.h"
#include "DeployServerGRPC/DeployerExportServiceImpl.h"
#include <mutex>


#define __MAX_CLIENT__ 128
#define __HANDLE_REQ_BUFF__ 48 //TODO: may need to add back in
#define __FAILED__ -1

#define __ALLOC_FAILED__ 0
#define __ALLOC_SUCCESS__ 1
#define __ALLOC_INIT__ 2

namespace ec {
    class Server {
    public:
//        Server(uint32_t server_id, ip4_addr _ip_address, uint16_t _port, std::vector<Agent *> &_agents);
        Server(uint32_t server_id, ip4_addr _ip_address, ports_t _ports, std::vector<Agent *> &_agents);
        ~Server() = default;
        void initialize_tcp();
        void initialize_udp();

        struct server_t {
            int32_t sock_fd;
            struct sockaddr_in addr;
        };
        struct serv_thread_args {
            serv_thread_args()              = default;
            ~serv_thread_args() {
                if(cliaddr) {
                    cliaddr = nullptr;
                }
            }
            serv_thread_args(int _clifd, struct sockaddr_in *_cliaddr)
                    : clifd(_clifd), cliaddr(_cliaddr) {}
            int clifd                       = 0;
            struct sockaddr_in *cliaddr     = nullptr;
        };

        [[noreturn]] void serve_tcp();
        [[noreturn]] void serve_udp();

//        virtual void serveGrpcDeployExport() = 0;

        void handle_client_reqs_tcp(void *clifd);
        void handle_client_reqs_udp(void *clifd);
        virtual int handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd) = 0;

        void incr_threads_created();
        int get_threads_created();
        void incr_threads_closed();
        int get_threads_closed();
        void incr_mod_counter();
        int get_mod_counter();
        //TODO: make return values error codes and pass struct via "msg_res"
//        int handle_req(const msg_t *req, msg_t *res, serv_thread_args* args);

//        int serve_add_cgroup_to_ec(const msg_t *req, msg_t *res, serv_thread_args* args);
//
//        int serve_cpu_req(const msg_t *req, msg_t *res, serv_thread_args* args);
//
//        int serve_mem_req(const msg_t *req, msg_t *res, serv_thread_args* args);


//        ip4_addr get_ip() { return ip_address; }
//        uint16_t get_port() { return port; }
        uint32_t get_server_id() const { return server_id; }
        std::mutex mtx;

        bool init_agent_connections();
    
    private:

        ip4_addr ip_address;
//        uint16_t port;
        ports_t ports;
        std::vector<Agent *> agents;

        //ECAPI *manager;
        struct server_t server_sock_tcp;
        struct server_t server_sock_udp;

        int server_initialized;
        uint32_t num_of_cli;
        int udp_threads_created;
        int udp_threads_closed;
        int mod_counter;
        std::mutex thr_created_mtx, thr_closed_mtx, mod_cntr_mtx;

    protected:
        uint32_t server_id;

//        ec::rpc::DeployerExportServiceImpl grpcServer;
    };
}


#endif //EC_GCM_SERVER_H

