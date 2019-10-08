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
#define NUM_AGENTS      1

int main() {


//    auto *gcm = new ec::GlobalCloudManager("128.138.244.104", GCM_PORT);
    auto *gcm = new ec::GlobalCloudManager("127.0.0.1", GCM_PORT);

    uint32_t ec_id = gcm->create_ec(NUM_AGENTS);
    std::cout << "ec_id: " << ec_id << std::endl;

    ec::ElasticContainer *ec = gcm->get_ec(ec_id);
    std::cout << "ec_id from ec: " << ec->get_ec_id() << std::endl;

    ec->build_ec_handler(SERVER_PORT);

    ec::Manager *m = ec->get_manager();
    ec::Server *s = m->get_server();

    std::cout <<  "(ec_id: " << m->get_ec_id() << ", ip: " << s->get_ip() << ", port: " << s->get_port() << ")" <<std::endl;

    s->initialize_server();
    s->serve();

//    ec->create_manager();
//    ec->create_server(SERVER_PORT);
////    ec->set

//    ec::Manager *m = ec->get_manager();
//    std::cout << "(ec_id: " << m->get_ec_id() << ", ip: " << m->get_ip() << ", port: " << m->get_port() << ")" <<std::endl;



//    ec::Manager::Server s = m->get_server();
//    std::cout << s.get_test_var() << std::endl;
//    std::cout << s.manager()->get_ip() << std::endl;
//
//    s.serve();

//    ec::Manager =


//    ec::Manager m = gcm->get_manager(ec_id);
//    std::cout << m.get_ec_id() << std::endl;

//    ec::Manager k = gcm->get_manager(2);
//    std::cout << m.get_ec_id() << std::endl;
//    gcm->



//    auto *m = new ec::Manager(MANAGER_ID);
//    std::cout << m->get_ec_id() << std::endl;
////
//    m->allocate_new_cgroup(CONTAINER_ID, SERVER_IP);
//    m->allocate_new_cgroup(CONTAINER_ID, "127.0.0.1");
//    std::cout << m->handle(CONTAINER_ID, SERVER_IP) << std::endl;
//    m->allocate_new_cgroup(CONTAINER_ID, SERVER_IP);
//
//    ec::k_msg_t k_msg(2130706433, 1, 0, 123456789, 1);
//
//    ec::msg_t msg(k_msg);
//
//    std::cout << k_msg << std::endl;
//    std::cout << msg << std::endl;

//    std::cout << k_msg << std::endl;


//
//    m->allocate_new_cgroup(2, "ip_2");




    return 0;
}