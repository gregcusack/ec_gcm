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

        uint32_t accept_container();       //probably want to return a FD for that new SubContainer thread
        void allocate_container(uint32_t cgroup_id, uint32_t server_ip);
        void allocate_container(uint32_t cgroup_id, std::string server_ip);

//        void serve(Manager *m);

        uint32_t handle(uint32_t cgroup_id, uint32_t server_ip);
//        void set_server(Server *_server) {server = _server;};


        //getters
        uint32_t get_ec_id() { return ec_id; }
        const container_map& get_containers() {return containers;}
        SubContainer* get_container(SubContainer::ContainerId &container_id);

        void set_server(Server *serv) { s = serv; }
        Server *get_server() { return s; }




    private:
        uint32_t ec_id;
//        ip4_addr ip_address;
//        uint16_t port;
        container_map containers;
        Server *s;

        //cpu
//        uint64_t runtime_remaining;


    };


}


#endif //EC_GCM_EC_MANAGER_H
