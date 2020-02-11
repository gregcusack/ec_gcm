//
// Created by greg on 9/16/19.
//

#include "Server.h"

ec::Server::Server(uint32_t _server_id, ec::ip4_addr _ip_address, uint16_t _port, std::vector<Agent *> &_agents)
    : server_id(_server_id), ip_address(_ip_address), port(_port), agents(_agents), server_initialized(false),
    agent_clients_({}) {}


void ec::Server::initialize() {
    int32_t addrlen, opt = 1;
    if((server_socket.sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cout << "[ERROR]: Server socket creation failed in server: " << server_id << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::cout << "[dgb]: Server socket fd: " << server_socket.sock_fd << std::endl;

    if(setsockopt(server_socket.sock_fd, SOL_SOCKET, SO_REUSEPORT, (char*)&opt, sizeof(opt))) {
        std::cout << "[ERROR]: EC Server id: " << server_id << ". Setting socket options failed!" << std::endl;
        exit(EXIT_FAILURE);
    }

    server_socket.addr.sin_family = AF_INET;
    server_socket.addr.sin_addr.s_addr = INADDR_ANY;
    server_socket.addr.sin_port = htons(port);

    if(bind(server_socket.sock_fd, (struct sockaddr*)&server_socket.addr, sizeof(server_socket.addr)) < 0) {
        std::cout << "[ERROR] EC Server id: " << server_id << ". Binding socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(listen(server_socket.sock_fd, 3) < 0) {
        std::cout << "[ERROR]: EC Server id: " << server_id << ". Listening on socket failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "[dgb]: EC Server id: " << server_id << ". socket successfully created!" << std::endl;

//    Create AgentClients
    if(!init_agent_connections()) {
        std::cout << "[ERROR Server] not all agents connected to server_id: " << server_id << "! Exiting..." << std::endl;
        exit(EXIT_FAILURE);
    }

//    manager = new Manager(server_id, agent_clients);
//    if(manager == nullptr) {
//        std::cout << "ERROR: Server initialized without manager. " << std::endl;
//        exit(EXIT_FAILURE);
//    }
    server_initialized = true; //server setup can run now
    
}

void ec::Server::serve() {
    std::cerr << "[dbg] Server::serve function begins!" << std::endl;
    if(!server_initialized) {
        std::cout << "ERROR: Server has not been initialized! Must call initialize() before serve()" << std::endl;
        exit(EXIT_FAILURE);
    }

    //test if server_socket struct valid here somehow
    fd_set readfds;
    int32_t max_sd, sd, cliaddr_len, clifd, select_rv;
//    int32_t num_of_cli = 0;
    std::cout << "[dbg] Server::serve(): line 65" << std:: endl;
    std::thread threads[__MAX_CLIENT__];
    serv_thread_args *args;
    FD_ZERO(&readfds);

    max_sd = server_socket.sock_fd + 1;
    cliaddr_len = sizeof(server_socket.addr);
    std::cout << "[dbg]: Max socket descriptor is: " << max_sd << std::endl;

    //std::cout << "[dgb]: All PIDs for the _ec reference: " << std::endl;
    
    while(true) {
        FD_SET(server_socket.sock_fd, &readfds);
//        std::cout << "[dgb]: In while loop waiting for server socket event. EC Server id: " << _ec->get_manager_id() << std::endl;
        //std::cout << "[dbg] serve: serve readfds: " << &readfds. << std::endl;
        std::cerr<< "[dgb] an event HAS NOT happened on the server socket. "<< std::endl;
        select_rv = select(max_sd, &readfds, nullptr, nullptr, nullptr);

        std::cerr<< "[dgb] an event HAS happened on the server socket. "<< std::endl;

        if(FD_ISSET(server_socket.sock_fd, &readfds)) {
            if((clifd = accept(server_socket.sock_fd, (struct sockaddr *)&server_socket.addr, (socklen_t*)&cliaddr_len)) > 0) {
                std::cout << "[dgb]: Container tried to request a connection. EC Server id: " << server_id << std::endl;
                args = new serv_thread_args();
                args->clifd = clifd;
                args->cliaddr = &server_socket.addr;
                std::cout << "server rx connection from clifd: " << clifd << std::endl;
//                std::cout << "num_cli: " << num_of_cli << std::endl;
                threads[num_of_cli] = std::thread(&Server::handle_client_reqs, this, (void*)args);
//                threads[num_of_cli] = std::thread(&Server::handle_client_reqs, this, (void*)&clifd);
            }
            else {
                std::cout << "[ERROR]: EC Server id: " << server_id << ". Unable to accept connection. "
                                                                            "Trying again. Error response: " << clifd << std::endl;
                continue;
            }
        }
    }

}

void ec::Server::handle_client_reqs(void *args) {
    ssize_t num_bytes;
    uint64_t ret;
    char buff_in[__BUFFSIZE__];
//    char *buff_out;
    auto *arguments = reinterpret_cast<serv_thread_args*>(args);
    int client_fd = arguments->clifd;

    num_of_cli++;
    bzero(buff_in, __BUFFSIZE__);
    while((num_bytes = read(client_fd, buff_in, __BUFFSIZE__)) > 0 ) {
        auto *req = reinterpret_cast<msg_t*>(buff_in);
        req->set_ip(arguments->cliaddr->sin_addr.s_addr); //this needs to be removed eventually
        auto *res = new msg_t(*req);
        ret = handle_req(req, res, arguments->cliaddr->sin_addr.s_addr, arguments->clifd);
//        std::cout << "Sending back: " << *res << std::endl;
        if(ret == __ALLOC_SUCCESS__) {  //TODO: fix this.
            if(write(client_fd, (const char*) &*res, sizeof(*res)) < 0) {
                    std::cout << "[ERROR]: EC Server id: " << server_id << ". Failed writing to socket" << std::endl;
                    break;
            }
        }
        else {
            if(write(client_fd, (const char*) &*res, sizeof(*res)) < 0) {
                std::cout << "[ERROR]: EC Server id: " << server_id << ". Failed writing to socket. Part 2" << std::endl;
                break;
            }
//            break;
        }
    }
}

int ec::Server::init_agent_connections() {
    int sockfd, i;
    struct sockaddr_in servaddr;
    int num_connections = 0;

//    for(i = 0; i < num_agents; i++) {
    for(const auto &ag : agents) {
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cout << "[ERROR]: GCM Socket creation failed. Agent is not up!" << std::endl;
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(ag->get_port());
        servaddr.sin_addr.s_addr = inet_addr((ag->get_ip()).to_string().c_str());

        if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            std::cout << "[ERROR] GCM: Connection to agent_clients failed. \n Agent on ip: " << ag->get_ip() << "is not connected" << std::endl;
            std::cout << "Are the agents up?" << std::endl;
        }
        else {
            num_connections++;
        }

        agent_clients_.push_back(new AgentClient(ag, sockfd));
        std::cout << "agent_clients sockfd: " << sockfd << ", " << agent_clients_[agent_clients_.size() - 1]->get_socket() << std::endl;
    }
    return num_connections == agent_clients_.size();

}


