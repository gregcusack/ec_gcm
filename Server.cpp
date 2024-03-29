//
// Created by greg on 9/16/19.
//

#include "Server.h"
#include "Agents/AgentClientDB.h"

ec::Server::Server(uint32_t _server_id, ec::ip4_addr _ip_address, ec::ports_t _ports, std::vector<Agent *> &_agents)
    : server_id(_server_id), ip_address(_ip_address), ports(_ports), agents(_agents), server_initialized(0),
      num_of_cli(0) {}

void ec::Server::initialize_tcp() {
    int32_t addrlen, opt = 1;
    num_of_cli = 0;
    if((server_sock_tcp.sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        SPDLOG_CRITICAL("[ERROR]: Server socket creation failed in server: {}", server_id);
        std::exit(EXIT_FAILURE);
    }

    SPDLOG_DEBUG("Server socket fd: {}", server_sock_tcp.sock_fd);
    if(setsockopt(server_sock_tcp.sock_fd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(opt))) {
        SPDLOG_CRITICAL("[ERROR]: EC Server id: {}. Setting socket options failed!", server_id);
        exit(EXIT_FAILURE);
    }

    server_sock_tcp.addr.sin_family = AF_INET;
    server_sock_tcp.addr.sin_addr.s_addr = INADDR_ANY;
    server_sock_tcp.addr.sin_port = htons(ports.tcp);

    if(bind(server_sock_tcp.sock_fd, (struct sockaddr*)&server_sock_tcp.addr, sizeof(server_sock_tcp.addr)) < 0) {
        SPDLOG_CRITICAL("[ERROR] EC Server id: {}. Binding socket failed", server_id);
        exit(EXIT_FAILURE);
    }

    if(listen(server_sock_tcp.sock_fd, 3) < 0) {
        SPDLOG_CRITICAL("[ERROR]: EC Server id: {}. Listening on socket failed", server_id);
        exit(EXIT_FAILURE);
    }
    SPDLOG_DEBUG("EC Server id: {}. socket successfully created!", server_id);

    server_initialized += 1; //server setup can run now
    
}

void ec::Server::initialize_udp() {
    int32_t addrlen, opt = 1;

    if((server_sock_udp.sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        SPDLOG_CRITICAL("[ERROR]: Server socket creation failed in server: {}", server_id);
        std::exit(EXIT_FAILURE);
    }

    SPDLOG_DEBUG("Server socket fd: {}", server_sock_udp.sock_fd);
    if(setsockopt(server_sock_udp.sock_fd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(opt))) {
        SPDLOG_CRITICAL("[ERROR]: EC Server id: {}. Setting socket options failed!", server_id);
        exit(EXIT_FAILURE);
    }

    server_sock_udp.addr.sin_family = AF_INET;
    server_sock_udp.addr.sin_addr.s_addr = INADDR_ANY;
    server_sock_udp.addr.sin_port = htons(ports.udp);

    if(bind(server_sock_udp.sock_fd, (struct sockaddr*)&server_sock_udp.addr, sizeof(server_sock_udp.addr)) < 0) {
        SPDLOG_CRITICAL("[ERROR] EC Server id: {}. Binding socket failed", server_id);
        exit(EXIT_FAILURE);
    }
    server_initialized += 1;
}


[[noreturn]] void ec::Server::serve_tcp() {
    if(server_initialized != 2) {
        SPDLOG_CRITICAL("ERROR: Server has not been initialized! Must call initialize_tcp() before serve_tcp()");
        exit(EXIT_FAILURE);
    }

    //test if server_sock_tcp struct valid here somehow
    fd_set readfds;
    int32_t max_sd, sd, cliaddr_len, clifd, select_rv;

    std::thread threads[__MAX_CLIENT__];
    FD_ZERO(&readfds);

    max_sd = server_sock_tcp.sock_fd + 1;
    cliaddr_len = sizeof(server_sock_tcp.addr);
    SPDLOG_DEBUG("[dbg]: Max socket descriptor is: {}", max_sd);


    while(true) {
        FD_SET(server_sock_tcp.sock_fd, &readfds);
        select_rv = select(max_sd, &readfds, nullptr, nullptr, nullptr);

        if(FD_ISSET(server_sock_tcp.sock_fd, &readfds)) {
            if((clifd = accept(server_sock_tcp.sock_fd, (struct sockaddr *)&server_sock_tcp.addr, (socklen_t*)&cliaddr_len)) > 0) {
                SPDLOG_DEBUG("===========================================");
                SPDLOG_DEBUG("Container tried to request a connection. EC Server id: {}", server_id);
                auto args = new serv_thread_args(clifd, &server_sock_tcp.addr);
                std::thread client_handler(&Server::handle_client_reqs_tcp, this, (void*)args);
                client_handler.detach();
            }
            else {
                SPDLOG_ERROR("[ERROR]: EC Server id: {}. Unable to accept connection. "
                             "Trying again. Error response: {}", server_id, clifd);
                continue;
            }
        }
    }

}


void ec::Server::handle_client_reqs_tcp(void *args) {
    ssize_t num_bytes;
    uint64_t ret;
    char buff_in[__HANDLE_REQ_BUFF__] = {0};

    auto *arguments = reinterpret_cast<serv_thread_args*>(args);
    int client_fd = arguments->clifd;
    auto client_ip = arguments->cliaddr->sin_addr.s_addr;
    delete arguments;
//    delete (serv_thread_args*)args;

    num_of_cli++;
    while((num_bytes = read(client_fd, buff_in, __HANDLE_REQ_BUFF__)) > 0 ) {
        auto *req = reinterpret_cast<msg_t*>(buff_in);
        req->set_ip_from_net(client_ip); //this needs to be removed eventually
        auto *res = new msg_t(*req);
        SPDLOG_TRACE("received: {}", *req);

        ret = handle_req(req, res, om::net::ip4_addr::from_net(client_ip).to_uint32(), client_fd);

        if(ret == __ALLOC_INIT__) {
            if (write(client_fd, (const char *) &*res, sizeof(*res)) < 0) {
                SPDLOG_ERROR("EC Server id: {}. Failed writing to socket", server_id);
                break;
            }
        }
        else if(ret == __ALLOC_SUCCESS__ && !res->request) {
            SPDLOG_TRACE("sending back alloc success!");
            if(write(client_fd, (const char*) &*res, sizeof(*res)) < 0) {
                SPDLOG_ERROR("EC Server id: {}. Failed writing to socket", server_id);
                break;
            }
            else {
                SPDLOG_DEBUG("sucess writing back to socket on mem resize!");
            }
        }
        else if(ret == __ALLOC_MEM_FAILED__) {
            if(write(client_fd, (const char*) &*res, sizeof(*res)) < 0) {
                SPDLOG_ERROR("EC Server id: {}. Failed writing to socket on mem failed", server_id);
                break;
            }
        }
        else if(ret == __ALLOC_FAILED__) {
            SPDLOG_ERROR("handle_req() failed!");
//            break;
        }
        delete res;
//        delete req;
    }
}

void ec::Server::serve_udp() {
    if(server_initialized != 2) {
        SPDLOG_CRITICAL("ERROR: Server has not been initialized! Must call initialize() before serve()");
        exit(EXIT_FAILURE);
    }

    //test if server_socket struct valid here somehow
    fd_set readfds;
    int32_t max_sd, sd, cliaddr_len, clifd, select_rv, msg_len;

    std::thread threads[__MAX_CLIENT__];
    FD_ZERO(&readfds);

    max_sd = server_sock_udp.sock_fd + 1;
    cliaddr_len = sizeof(server_sock_udp.addr);
    SPDLOG_DEBUG("[dbg]: Max socket descriptor is: {}", max_sd);


    while(true) {
        FD_SET(server_sock_udp.sock_fd, &readfds);
        select_rv = select(max_sd, &readfds, nullptr, nullptr, nullptr);

        if(FD_ISSET(server_sock_udp.sock_fd, &readfds)) {
            auto args = new serv_thread_args(server_sock_udp.sock_fd, &server_sock_udp.addr);
            handle_client_reqs_udp((void*)args);
//            std::thread client_handler(&Server::handle_client_reqs_udp, this, (void*)args);
//            client_handler.detach();
        }
    }
}

void ec::Server::handle_client_reqs_udp(void *args) {
    ssize_t num_bytes;
    uint64_t ret;
    int err;
    char buff_in[__HANDLE_REQ_BUFF__] = {0};

    auto *arguments = reinterpret_cast<serv_thread_args*>(args);
    int client_fd = arguments->clifd;
    auto client_ip = arguments->cliaddr->sin_addr.s_addr;
    auto port = ntohs(arguments->cliaddr->sin_port);
    auto cliaddr = &arguments->cliaddr;
    int len = sizeof(cliaddr);
    delete arguments;
//    delete (serv_thread_args*)args;

    num_of_cli++;
//    incr_threads_created();
    num_bytes = recvfrom(client_fd, buff_in, sizeof(buff_in), 0, (struct sockaddr*)&cliaddr, (socklen_t *)&len);
    if(num_bytes < 0) {
        SPDLOG_ERROR("EC Server id: {}. Failed reading from udp socket", server_id);
    }

    auto *req = reinterpret_cast<msg_t*>(buff_in);
    req->set_ip_from_host(req->client_ip.to_uint32()); //this needs to be removed eventually
    auto *res = new msg_t(*req);
//    SPDLOG_INFO("req from server_id: {} w/ port: {} is: {}", server_id, port, *req);
    ret = handle_req(req, res, om::net::ip4_addr::from_net(client_ip).to_uint32(), client_fd);

    if(!res->request && ret == __ALLOC_SUCCESS__) {
        SPDLOG_TRACE("sending back alloc success!");
        if(send(client_fd, (const char*) &*res, sizeof(*res), MSG_CONFIRM) < 0) {
            SPDLOG_ERROR("EC Server id: {}. Failed writing to socket", server_id);
        }
        else {
            SPDLOG_DEBUG("sucess writing back to socket on mem resize!");
        }
    }
    else if(ret == __ALLOC_FAILED__) {
        SPDLOG_ERROR("handle_req() failed!");
    }
    delete res;
}
