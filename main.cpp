#include <iostream>

#include "GlobalCloudManager.h"
#include "Manager.h"
#include "SubContainer.h"
#include "types/msg.h"


#define MANAGER_ID      23
#define CONTAINER_ID    1
#define SERVER_IP       2130706433      //127.0.0.1
#define GCM_PORT        8888             //Not sure if we need a port here tbh
#define SERVER_PORT     4444

int main() {
    std::vector<std::string>  agent_ips{"127.0.0.1"};//), "127.0.0.1", "127.0.0.1"};

//    auto *gcm = new ec::GlobalCloudManager("128.138.244.104", GCM_PORT);
    auto *gcm = new ec::GlobalCloudManager("127.0.0.1", GCM_PORT, agent_ips);


    uint32_t ec_id = gcm->create_ec();
    std::cout << "ec_id: " << ec_id << std::endl;

    ec::ElasticContainer *ec = gcm->get_ec(ec_id);
    std::cout << "ec_id from ec: " << ec->get_ec_id() << std::endl;

    ec->build_ec_handler(SERVER_PORT);

    ec::Manager *m = ec->get_manager();
    ec::Server *s = m->get_server();

    std::cout <<  "(ec_id: " << m->get_ec_id() << ", ip: " << s->get_ip() << ", port: " << s->get_port() << ")" <<std::endl;

    s->initialize_server();
    s->serve();

//    delete agents;
    delete gcm;


    return 0;
}
