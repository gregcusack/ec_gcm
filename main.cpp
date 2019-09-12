#include <iostream>

#include "manager.h"
#include "container.h"

#define MANAGER_ID      23
#define CONTAINER_ID    1
#define SERVER_IP       "ip"

int main() {


    auto *m = new ec::manager(MANAGER_ID);
    std::cout << m->get_ec_id() << std::endl;
//
    m->allocate_new_cgroup(CONTAINER_ID, SERVER_IP);
    std::cout << m->handle(CONTAINER_ID, SERVER_IP) << std::endl;
    m->allocate_new_cgroup(CONTAINER_ID, SERVER_IP);


//
//    m->allocate_new_cgroup(2, "ip_2");




    return 0;
}