//
// Created by greg on 9/16/19.
//

#include "Server.h"
#include "Agents/AgentClientDB.h"

ec::Server::Server(uint32_t _server_id, ec::ip4_addr _ip_address, uint16_t _port, std::vector<Agent *> &_agents)
    : server_id(_server_id), ip_address(_ip_address), port(_port), agents(_agents), server_initialized(false),
    num_of_cli(0) {}//, grpcServer(rpc::DeployerExportServiceImpl()) {}


void ec::Server::initialize() {
    int32_t addrlen, opt = 1;
    num_of_cli = 0;
    if((server_socket.sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cout << "[ERROR]: Server socket creation failed in server: " << server_id << std::endl;
        std::exit(EXIT_FAILURE);
    }
#ifndef DEBUG
    std::cout << "[dgb]: Server socket fd: " << server_socket.sock_fd << std::endl;
#endif
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

    server_initialized = true; //server setup can run now
    
}

void ec::Server::serve() {
    if(!server_initialized) {
        std::cout << "ERROR: Server has not been initialized! Must call initialize() before serve()" << std::endl;
        exit(EXIT_FAILURE);
    }

    //test if server_socket struct valid here somehow
    fd_set readfds;
    int32_t max_sd, sd, cliaddr_len, clifd, select_rv;

    std::thread threads[__MAX_CLIENT__];
    FD_ZERO(&readfds);

    max_sd = server_socket.sock_fd + 1;
    cliaddr_len = sizeof(server_socket.addr);
    std::cout << "[dbg]: Max socket descriptor is: " << max_sd << std::endl;

    //std::cout << "[dgb]: All PIDs for the _ec reference: " << std::endl;
    
    while(true) {
        FD_SET(server_socket.sock_fd, &readfds);
        select_rv = select(max_sd, &readfds, nullptr, nullptr, nullptr);

        if(FD_ISSET(server_socket.sock_fd, &readfds)) {
            if((clifd = accept(server_socket.sock_fd, (struct sockaddr *)&server_socket.addr, (socklen_t*)&cliaddr_len)) > 0) {
                std::cout << "=================================================================================================" << std::endl;
                std::cout << "[SERVER DBG]: Container tried to request a connection. EC Server id: " << server_id << std::endl;
                auto args = new serv_thread_args(clifd, &server_socket.addr);
                std::thread client_handler(&Server::handle_client_reqs, this, (void*)args);
                client_handler.detach();
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
    char buff_in[__HANDLE_REQ_BUFF__] = {0};
//    char *buff_out;
    auto *arguments = reinterpret_cast<serv_thread_args*>(args);
    int client_fd = arguments->clifd;
    auto client_ip = arguments->cliaddr->sin_addr.s_addr;
    delete arguments;

    num_of_cli++;
    while((num_bytes = read(client_fd, buff_in, __HANDLE_REQ_BUFF__)) > 0 ) {
//        std::cout << "num bytes read: " << num_bytes << std::endl;
        auto *req = reinterpret_cast<msg_t*>(buff_in);
        req->set_ip_from_net(client_ip); //this needs to be removed eventually
        auto *res = new msg_t(*req);
#ifndef DEBUG_MAX
        std::cout << "received: " << *req << std::endl;
#endif
        ret = handle_req(req, res, om::net::ip4_addr::from_net(client_ip).to_uint32(), client_fd);

        if(ret == __ALLOC_INIT__) { 
            if (write(client_fd, (const char *) &*res, sizeof(*res)) < 0) {
                std::cout << "[ERROR]: EC Server id: " << server_id << ". Failed writing to socket" << std::endl;
                break;
            }
        }
	    else if(ret == __ALLOC_SUCCESS__ && !res->request) {
            std::cout << "sending back alloc success!" << std::endl;
            if(write(client_fd, (const char*) &*res, sizeof(*res)) < 0) {
                std::cerr << "[ERROR]: EC Server id: " << server_id << ". Failed writing to socket" << std::endl;
                break;
            }
#ifndef DEBUG
            else {
                std::cout << "sucess writing back to socket on mem resize!" << std::endl;
            }
#endif
        }
		else if(ret == __ALLOC_MEM_FAILED__) {
            if(write(client_fd, (const char*) &*res, sizeof(*res)) < 0) {
                std::cerr << "[ERROR]: EC Server id: " << server_id << ". Failed writing to socket on mem failed" << std::endl;
                break;
            }
        }
        else if(ret == __ALLOC_FAILED__) {
            std::cerr << "[ERROR]: handle_req() failed!" << std::endl;
            break;
        }
        delete res;
    }
}

bool ec::Server::init_agent_connections() {
    int sockfd, i;
    struct sockaddr_in servaddr;
    uint32_t num_connections = 0;
    AgentClientDB* agent_clients_db = AgentClientDB::get_agent_client_db_instance();
//    for(i = 0; i < num_agents; i++) {
    for(const auto &ag : agents) {
        if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            std::cerr << "[ERROR]: GCM Socket creation failed. Agent is not up!" << std::endl;
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(ag->get_port());
        servaddr.sin_addr.s_addr = inet_addr((ag->get_ip()).to_string().c_str());
        std::cout << "ag->ip: " << ag->get_ip() << std::endl;

        if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            std::cerr << "[ERROR] GCM: Connection to agent_clients failed. \n Agent on ip: " << ag->get_ip() << "is not connected" << std::endl;
            std::cerr << "Are the agents up?" << std::endl;
        }
        else {
            num_connections++;
        }
        auto* ac = new AgentClient(ag, sockfd);
        agent_clients_db->add_agent_client(ac);
#ifndef DEBUG_MAX
        std::cout << "[dbg] Agent client added to db and agent_clients sockfd: " << sockfd << ", " << agent_clients_db->get_agent_client_by_ip(ag->get_ip())->get_socket()
        <<" agent db size is: " << agent_clients_db->get_agent_clients_db_size()<< std::endl;
#endif
    }
    return num_connections == agent_clients_db->get_agent_clients_db_size();

}

//void ec::Server::serveGrpcDeployExport() {
//    std::string server_addr("10.0.2.15:4447");
//    rpc::DeployerExportServiceImpl service;
//    grpc::ServerBuilder builder;
//
//    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
//    builder.RegisterService(&service);
//    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
//
//    std::cout << "Grpc Server listening on: " << server_addr << std::endl;
//    server->Wait();
//
//}



