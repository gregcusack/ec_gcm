//
// Created by greg on 9/16/19.
//

#include "Server.h"

ec::Server::Server(ip4_addr _ip_address, uint16_t _port)
        : ip_address(_ip_address), port(_port), m(nullptr),
        mem_reqs(0), cpu_limit(500000), memory_limit(30000),
        server_initialized(false), test(23), num_of_cli(0) {

    //TODO: initialize agents

}

void ec::Server::initialize_server() {
    if(m == nullptr) {
        std::cout << "ERROR: Server initialized without manager. " << std::endl;
        exit(EXIT_FAILURE);
    }
    int32_t addrlen, opt = 1;
    if((server_socket.sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cout << "[ERROR]: Server socket creation failed in EC: " << m->get_ec_id() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "[dgb]: Server socket fd: " << server_socket.sock_fd << std::endl;

    if(setsockopt(server_socket.sock_fd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(opt))) {
        std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Setting socket options failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    server_socket.addr.sin_family = AF_INET;
    server_socket.addr.sin_addr.s_addr = INADDR_ANY;
    server_socket.addr.sin_port = htons(port);

    if(bind(server_socket.sock_fd, (struct sockaddr*)&server_socket.addr, sizeof(server_socket.addr)) < 0) {
        std::cout << "[ERROR] EC Server id: " << m->get_ec_id() << ". Binding socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(listen(server_socket.sock_fd, 3) < 0) {
        std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Listening on socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "[dgb]: EC Server id: " << m->get_ec_id() << ". socket successfully created!" << std::endl;

    server_initialized = true; //server setup can run now

    if(!init_agents_connection(m->get_num_agents())) {
        std::cout << "Agent initialization failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Agent conenctions successful!" << std::endl;

}

//We may want to make this a separate process???
/*!
 * TODO: turn pthreads into std::threads
 */
void ec::Server::serve() {
    if(!server_initialized) {
        std::cout << "ERROR: Server has not been initialized! Must call initialize_server() before serve()" << std::endl;
        exit(EXIT_FAILURE);
    }

    //test if server_socket struct valid here somehow
    fd_set readfds;
    //struct sockaddr_in cliaddr; -- this is server_socket.adder
    int32_t max_sd, sd, cliaddr_len, clifd, select_rv;
//    int32_t num_of_cli = 0;
    std::thread threads[__MAX_CLIENT__];
    serv_thread_args *args;
    FD_ZERO(&readfds);

    max_sd = server_socket.sock_fd + 1;
    cliaddr_len = sizeof(server_socket.addr);
    std::cout << "[dbg]: Max socket descriptor is: " << max_sd << std::endl;

    while(true) {
        FD_SET(server_socket.sock_fd, &readfds);
//        std::cout << "[dgb]: In while loop waiting for server socket event. EC Server id: " << m->get_ec_id() << std::endl;

        select_rv = select(max_sd, &readfds, nullptr, nullptr, nullptr);

//        std::cout << "[dgb] an even happened on the server socket. EC Server id: " << m->get_ec_id() << std::endl;

        if(FD_ISSET(server_socket.sock_fd, &readfds)) {
            if((clifd = accept(server_socket.sock_fd, (struct sockaddr *)&server_socket.addr, (socklen_t*)&cliaddr_len)) > 0) {
                std::cout << "[dgb]: Container tried to request a connection. EC Server id: " << m->get_ec_id() << std::endl;
                args = new serv_thread_args();
                args->clifd = clifd;
                args->cliaddr = &server_socket.addr;
                std::cout << "server rx connection from clifd: " << clifd << std::endl;
//                std::cout << "num_cli: " << num_of_cli << std::endl;
                threads[num_of_cli] = std::thread(&Server::handle_client_reqs, this, (void*)args);
//                threads[num_of_cli] = std::thread(&Server::handle_client_reqs, this, (void*)&clifd);
            }
            else {
                std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Unable to accept connection. "
                                                                            "Trying again. Error response: " << clifd << std::endl;
                continue;
            }
        }
    }

}

void ec::Server::handle_client_reqs(void *args) {
    ssize_t num_bytes;
    uint64_t ret;
    char buffer[__BUFFSIZE__];
    auto *arguments = reinterpret_cast<serv_thread_args*>(args);
    int client_fd = arguments->clifd;

//    std::cout << "[SUCCESS] Server id: " << m->get_ec_id() << ". Server thread! New connection created. "
//                                                              "new socket fd: " << client_fd << std::endl;
    num_of_cli++;
    bzero(buffer, __BUFFSIZE__);
    while((num_bytes = read(client_fd, buffer, __BUFFSIZE__)) > 0 ) {
        ret = 0;
//        std::cout << "[dbg] Number of bytes read: " << num_bytes << std::endl;

        //moved up from handle_req
        auto *req = reinterpret_cast<msg_t*>(buffer);
        req->set_ip(arguments->cliaddr->sin_addr.s_addr); //this needs to be removed eventually
        auto *res = new msg_t(*req);

        ret = handle_req(req, res, arguments);
        if(ret == __ALLOC_SUCCESS__) {  //TODO: fix this.
            //for testing
            //res->rsrc_amnt -= 99999;
//            std::cout << "sending back: " << *res << std::endl;
            if(write(client_fd, (const char*) &*res, sizeof(*res)) < 0) {
                    std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Failed writing to socket" << std::endl;
                    break;
            }
        }
        else {
//            std::cout << "[FAILED] Error code: [" << ret << "]: EC Server id: " << m->get_ec_id() << ". Server thread: " << mem_reqs++ << std::endl;
            if(write(client_fd, (const char*) &*res, sizeof(*res)) < 0) {
                std::cout << "[ERROR]: EC Server id: " << m->get_ec_id() << ". Failed writing to socket. Part 2" << std::endl;
                break;
            }
//            break;
        }

    }
//    std::thread(ex)
//    std::terminate();
//    pthread_exit(nullptr);
}

int ec::Server::handle_req(const msg_t *req, msg_t *res, serv_thread_args* args) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in handle_req()" << std::endl;
        exit(EXIT_FAILURE);
    }

//    std::cout << "req: " << *req << std::endl;
    uint64_t ret = __FAILED__;
    std::cout << "req->req_type: " << req->req_type << std::endl;

    switch(req -> req_type) {
        case _MEM_:
//            std::cout << "[dbg]: EC Server id: " << m->get_ec_id() << ".Handling Mem request" << std::endl;
            ret = serve_mem_req(req, res, args);
            break;
        case _CPU_:
//            std::cout << "[dbg]: EC Server id: " << m->get_ec_id() << ".Handling CPU request" << std::endl;
            ret = serve_cpu_req(req, res, args);
            break;
        case _INIT_:
//            std::cout << "[dbg]: EC Server id: " << m->get_ec_id() << ".Handling INIT request" << std::endl;
            ret = serve_add_cgroup_to_ec(req, res, args);
            break;
        case _SLICE_:
            ret = serve_acquire_slice(req, res, args);
            break;
        default:
            std::cout << "[Error]: EC Server id: " << m->get_ec_id() << ". Handling memory/cpu request failed!" << std::endl;
    }
    return ret;

}

//int ec::Server::serve_add_cgroup_to_ec()

int ec::Server::serve_add_cgroup_to_ec(const ec::msg_t *req, ec::msg_t *res, serv_thread_args* args) {
    if(!req || !res || !args) {
        std::cout << "req, res, or args == null in serve_add_cgroup_to_ec()" << std::endl;
        return __FAILED__;
    }
//    mtx.lock(); //TODO: check if we actually need to lock here
    int ret = m->handle_add_cgroup_to_ec(res, req->cgroup_id, args->cliaddr->sin_addr.s_addr, args->clifd);
//    mtx.unlock();
    return ret;
}

//Logic in Manager
int ec::Server::serve_cpu_req(const msg_t *req, msg_t *res, serv_thread_args* args) {
    if(req == nullptr || res == nullptr) {
        std::cout << "req or res == null in serve_cpu_req()" << std::endl;
        return __FAILED__;
    }
    int ret = m->handle_bandwidth(req, res);
    if(ret != __ALLOC_SUCCESS__) {
        return __ALLOC_FAILED__;
    }
    return __ALLOC_SUCCESS__;
}

int ec::Server::serve_acquire_slice(const ec::msg_t *req, ec::msg_t *res, serv_thread_args *args) {
    if (req == nullptr || res == nullptr || args == nullptr) {
        std::cout << "req, res, or args == null in serve_acquire_slice()" << std::endl;
        return __FAILED__;
    }
    int ret = m->handle_slice_req(req, res, args->clifd);
    std::cout << "server slice returning slice: " << res->rsrc_amnt << std::endl;
    if (ret != __ALLOC_SUCCESS__) {
        return __ALLOC_FAILED__;
    }
    return __ALLOC_SUCCESS__;
}


int ec::Server::serve_mem_req(const msg_t *req, msg_t *res, serv_thread_args* args) {
    if (req == nullptr || res == nullptr || args == nullptr) {
        std::cout << "req, res, or args == null in serve_mem_req()" << std::endl;
        return __FAILED__;
    }
    int ret = m->handle_mem_req(req, res, args->clifd);
    if (ret != __ALLOC_SUCCESS__) {
        return __ALLOC_FAILED__;
    }
    return __ALLOC_SUCCESS__;
}

int ec::Server::init_agents_connection(int num_agents) {
    int sockfd, i;
    struct sockaddr_in servaddr;
    int num_connections = 0;

    for(i = 0; i < num_agents; i++) {
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cout << "[ERROR]: GCM Socket creation failed. Agent is not up!" << std::endl;
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(m->get_agents()[i]->port);
        servaddr.sin_addr.s_addr = inet_addr((m->get_agents()[i]->ip).to_string().c_str());

        if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            std::cout << "[ERROR] GCM: Connection to agent failed. \n Agent on ip: " << m->get_agents()[i]->ip << "is not connected" << std::endl;
            std::cout << "Are the agents up?" << std::endl;
        }
        else {
            num_connections++;
        }

        m->get_agents()[i]->sockfd = sockfd;
        std::cout << "agent sockfd: " << sockfd << ", " << m->get_agents()[i]->sockfd << std::endl;
    }
    return num_connections == num_agents;
}


































