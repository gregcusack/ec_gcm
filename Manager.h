//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_MANAGER_H
#define EC_GCM_MANAGER_H
#include "ElasticContainer.h"
//#include "Server.h"
//#include "SubContainer.h"
#include "types/msg.h"
#include "Agent.h"
#include "om.h"

namespace ec {
    class Manager {
    public:
//        Manager(uint32_t _ec_id, ip4_addr _ip_address, uint16_t _port, std::vector<Agent *> &_agents);
        Manager(uint32_t _ec_id, std::vector<Agent *> &_agents);
        ~Manager();
        //creates _ec and server and connects them
//        void build_manager_handler();
        void create_ec();


        const ElasticContainer& get_elastic_container() const;

        int handle_add_cgroup_to_ec(msg_t *res, uint32_t cgroup_id, uint32_t ip, int fd);


        uint32_t get_manager_id() { return manager_id; };

//        void set_server(Server *serv) { server = serv; }
//        Server *get_server() { return server; }




    private:
        uint32_t manager_id;
//        ip4_addr ip_address;
//        uint16_t port;
        ElasticContainer *_ec;
//        Server *server;

        //passed by reference from GlobalCloudManager
	    std::vector<Agent *> agents;



    };
}



#endif //EC_GCM_MANAGER_H
