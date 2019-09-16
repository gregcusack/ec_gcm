//
// Created by greg on 9/11/19.
//

#include "Manager.h"


ec::Manager::Manager(uint32_t _ec_id) : ec_id(_ec_id), s(nullptr) {}

void ec::Manager::allocate_container(uint32_t cgroup_id, uint32_t server_ip) {

    auto *c = new ec::SubContainer(cgroup_id, server_ip, ec_id);
    if (containers.find(*c->get_id()) != containers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        return;
    }
    std::cout << "Inserting (" << *c->get_id() << ")" << std::endl;
    containers.insert({*c->get_id(), c});
}

void ec::Manager::allocate_container(uint32_t cgroup_id, std::string server_ip) {

    auto *c = new ec::SubContainer(cgroup_id, std::move(server_ip), ec_id);
    if (containers.find(*c->get_id()) != containers.end()) {
        std::cout << "This SubContainer already exists! Can't allocate identical one!" << std::endl;
        return;
    }
    std::cout << "Inserting (" << *c->get_id() << ")" << std::endl;
    containers.insert({*c->get_id(), c});
}

uint32_t ec::Manager::handle(uint32_t cgroup_id, uint32_t server_ip) {
    auto c_id = ec::SubContainer::ContainerId(cgroup_id, ip4_addr::from_host(server_ip), ec_id);
    if (containers.find(c_id) != containers.end()) {
        return 0;
    }
    return 1;
}

ec::SubContainer *ec::Manager::get_container(ec::SubContainer::ContainerId &container_id) {
    auto itr = containers.find(container_id);
    if(itr == containers.end()) {
        std::cout << "ERROR: No EC with ec_id: " << ec_id << ". Exiting...." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return itr->second;
}



//void ec::Manager::serve() {
//    server = new ec::Manager::Server();
//////    server->get_manager();
//}

/*!
 * @struct server
 * @brief handles all connections
 */
//ec::Manager::Server::Server(Manager *_m)
//    : m(_m), mem_reqs(0), cpu_limit(500000), memory_limit(30000) {
//    test = 23;
//    int32_t addrlen, opt = 1;
//    if((server_socket.sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//        std::cout << "[ERROR]: Server socket creation failed in EC: " << m->get_ec_id() << std::endl;
//        std::exit(EXIT_FAILURE);
//    }
//    std::cout << "[dgb]: Server socket fd: " << server_socket.sock_fd << std::endl;
//
//    if(setsockopt(server_socket.sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char*)&opt, sizeof(opt))) {
//        std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Setting socket options failed!" << std::endl;
//        exit(EXIT_FAILURE);
//    }
//
//    server_socket.addr.sin_family = AF_INET;
//    server_socket.addr.sin_addr.s_addr = INADDR_ANY;
//    server_socket.addr.sin_port = htons(m->get_port());
//
//    if(bind(server_socket.sock_fd, (struct sockaddr*)&server_socket.addr, sizeof(server_socket.addr)) < 0) {
//        std::cout << "[ERROR] EC Server id: " << m->get_ec_id() << ". Binding socket failed" << std::endl;
//        exit(EXIT_FAILURE);
//    }
//
//    if(listen(server_socket.sock_fd, 3) < 0) {
//        std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Listening on socket failed" << std::endl;
//        exit(EXIT_FAILURE);
//    }
//    std::cout << "[dgb]: EC Server id: " << m->get_ec_id() << ". socket successfully created!" << std::endl;
//
//}
//
////We may want to make this a separate process???
///*!
// * TODO: turn pthreads into std::threads
// */
//void ec::Manager::Server::serve() {
//    //test if server_socket struct valid here somehow
//    fd_set readfds;
//    int32_t max_sd, sd, cliaddr_len, clifd, select_rv;
//    int32_t num_of_cli = 0;
//    pthread_t threads[__MAX_CLIENT__];
//    FD_ZERO(&readfds);
//
//    max_sd = server_socket.sock_fd + 1;
//    cliaddr_len = sizeof(server_socket.addr);
//    std::cout << "[dbg]: Max socket descriptor is: " << max_sd << std::endl;
//
//    while(true) {
//        FD_SET(server_socket.sock_fd, &readfds);
//        std::cout << "[dgb]: In while loop waiting for server socket event. EC Server id: " << m->get_ec_id() << std::endl;
//
//        select_rv = select(max_sd, &readfds, nullptr, nullptr, nullptr);
//
//        std::cout << "[dgb] an even happened on the server socket. EC Server id: " << m->get_ec_id() << std::endl;
//
//        if(FD_ISSET(server_socket.sock_fd, &readfds)) {
//            if((clifd = accept(server_socket.sock_fd, (struct sockaddr *)&server_socket.addr, (socklen_t*)&cliaddr_len)) > 0) {
//                std::cout << "[dgb]: Container tried to request a connection. EC Server id: " << m->get_ec_id() << std::endl;
//                if(pthread_create(&threads[num_of_cli], nullptr, handle_client_helper, (void*)&clifd)) {
//                    std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Unable to create server thread" << std::endl;
//                    break;
////                    exit(EXIT_FAILURE);
//                }
//            }
//            else {
//                std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Unable to accept connection. "
//                                                                            "Trying again. Error response: " << clifd << std::endl;
//                continue;
//            }
//        }
//    }
//
//}
//
//void *ec::Manager::Server::handle_client_reqs(void *clifd) {
//    ssize_t num_bytes;
//    int64_t ret;
//    char buffer[__BUFFSIZE__];
//    int32_t client_fd = *((int*) clifd);
//
//    std::cout << "[SUCCESS] Server id: " << m->get_ec_id() << ". Server thread! New connection created. "
//                                                           "new socket fd: " << client_fd << std::endl;
//
//    bzero(buffer, __BUFFSIZE__);
//    while((num_bytes = read(client_fd, buffer, __BUFFSIZE__)) > 0 ) {
//
//        ret = 0;
//        std::cout << "[dbg] Number of bytes read: " << num_bytes << std::endl;
//        ret = handle_req(buffer);
//        if(ret > 0) {
//            if(write(client_fd, (const char*) &ret, sizeof(uint32_t)) < 0) {
//                std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Failed writing to socket" << std::endl;
//                break;
//            }
//        }
//        else {
//            std::cout << "[FAILED]: EC Server id: " << m->get_ec_id() << ". Server thread: " << mem_reqs++ << std::endl;
//            if(write(client_fd, (const char*) &ret, sizeof(uint32_t)) < 0) {
//                std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Failed writing to socket. Part 2" << std::endl;
//                break;
//            }
//            break;
//        }
//
//    }
//    pthread_exit(nullptr);
//}
//
//void *ec::Manager::Server::handle_client_helper(void *clifd) {
//    return ((Server *)clifd)->handle_client_reqs(clifd);
//}
//
//int64_t ec::Manager::Server::handle_req(char *buffer) {
//    auto *req = (k_msg_t*)buffer;
//    msg_t msg(*req);
//    int64_t ret = __FAILED__;
//
//    switch(req -> is_mem) {
//        case true:
//            std::cout << "[dbg]: EC Server id: " << m->get_ec_id() << ".Handling Mem request" << std::endl;
//            ret = handle_mem_req(&msg);
//            break;
//        case false:
//            std::cout << "[dbg]: EC Server id: " << m->get_ec_id() << ".Handling CPU request" << std::endl;
//            ret = handle_cpu_req(&msg);
//            std::cout << "[dbg] EC Server id: " << m->get_ec_id() << ". Return CPU Request: " << ret << std::endl;
//            break;
//        default:
//            std::cout << "[Error]: EC Server id: " << m->get_ec_id() << ". Handling memory/cpu request failed!" << std::endl;
//    }
//    return ret;
//
//}
//
//int64_t ec::Manager::Server::handle_cpu_req(msg_t *req) {
//    int64_t ret = 0;
//    int64_t fail = 1;
//    pthread_mutex_t cpulock = PTHREAD_MUTEX_INITIALIZER;
//
//    if(req->is_mem) { return __FAILED__; }
//    if(cpu_limit > 0) {
//        pthread_mutex_lock(&cpulock);
//        ret = cpu_limit > __QUOTA__ ? __QUOTA__ : cpu_limit;
//        cpu_limit -= ret;
//        pthread_mutex_unlock(&cpulock);
//        return req->rsrc_amnt + ret;
//
//    }
//    else {
//        return fail;
//    }
//}
//
//int64_t ec::Manager::Server::handle_mem_req(msg_t *req) {
//    int64_t ret = 0;
//    int64_t fail = 1;
//    pthread_mutex_t memlock = PTHREAD_MUTEX_INITIALIZER;
//    if(!req->is_mem) { return __FAILED__; }
//
//    if(memory_limit > 0) {
//        pthread_mutex_lock(&memlock);
//        ret = memory_limit > __QUOTA__ ? __QUOTA__ : memory_limit;
//        memory_limit -= ret;
//        pthread_mutex_lock(&memlock);
//        return req->rsrc_amnt + ret;
//    }
//    else {
//        return fail;
//    }
//}
//
//
//
//
//
