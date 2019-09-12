#include <iostream>

#include "Manager.h"
#include "SubContainer.h"
#include "types/msg.h"


#define MANAGER_ID      23
#define CONTAINER_ID    1
#define SERVER_IP       2130706433      //127.0.0.1

int main() {


    auto *m = new ec::Manager(MANAGER_ID);
    std::cout << m->get_ec_id() << std::endl;
//
    m->allocate_new_cgroup(CONTAINER_ID, SERVER_IP);
    m->allocate_new_cgroup(CONTAINER_ID, "127.0.0.1");
    std::cout << m->handle(CONTAINER_ID, SERVER_IP) << std::endl;
    m->allocate_new_cgroup(CONTAINER_ID, SERVER_IP);

    ec::k_msg_t k_msg(2130706433, 1, 0, 123456789, 1);

    ec::msg_t msg(k_msg);

    std::cout << k_msg << std::endl;
    std::cout << msg << std::endl;

//    std::cout << k_msg << std::endl;


//
//    m->allocate_new_cgroup(2, "ip_2");




    return 0;
}