#include <iostream>

#include "GlobalCloudManager.h"
#include "ElasticContainer.h"
#include "SubContainer.h"
#include "types/msg.h"
#include <sys/wait.h>


#define MANAGER_ID      23
#define CONTAINER_ID    1
#define SERVER_IP       2130706433      //127.0.0.1
#define GCM_PORT        8888             //Not sure if we need a port here tbh
#define SERVER_PORT     4444

int main(){
    std::vector<std::string>    agent_ips{"127.0.0.1"};//), "127.0.0.1", "127.0.0.1"};
    std::vector<uint16_t>       server_ports{4444};

//    auto *gcm = new _ec::GlobalCloudManager("128.138.244.104", GCM_PORT);
    auto *gcm = new ec::GlobalCloudManager("127.0.0.1", GCM_PORT, agent_ips, server_ports);
    if(!gcm->init_agent_connections()) {
        std::cout << "[ERROR GCM] not all agents connected! Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }

    for(const auto &i : server_ports) {
        gcm->create_server();
    }
    std::cout << "num servers: " << gcm->get_servers().size() << std::endl;

    gcm->run();

    delete gcm;

/*
//    uint32_t manager_id = gcm->create_server();

//    std::cout << "manager_id: " << manager_id << std::endl;

//    _ec::Manager *_ec = gcm- >get_manager(manager_id);

//    std::cout << "manager_id from _ec: " << _ec->get_ec_id() << std::endl;
//
//    _ec->build_manager_handler();
//
//    _ec::ElasticContainer *_ec = _ec->get_elastic_container();
//    _ec::Server *s = _ec->get_server();
//
//    std::cout <<  "(manager_id: " << _ec->get_manager_id() << ", ip: " << s->get_ip() << ", port: " << s->get_port() << ")" <<std::endl;
//
//    s->initialize_server();
//    s->serve();

//    delete agents;
 */



    return 0;
}
