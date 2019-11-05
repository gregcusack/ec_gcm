//
// Created by Greg Cusack on 10/4/19.
//


#include "../types/msg.h"
#include "../om.h"

int main() {

    auto *msg_req = new ec::msg_t();
    msg_req->client_ip = om::net::ip4_addr::from_string("127.0.0.1");
    msg_req->cgroup_id = 23;
    msg_req->req_type = 2;
    msg_req->rsrc_amnt = 10000000;
    msg_req->request = 1;

//    auto *msg_res = new _ec::msg_t();
    auto msg_res = ec::msg_t();

    std::cout << "msg_req: " << *msg_req << std::endl;
    //std::cout << "msg_res: " << *msg_res << std::endl;

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(4444);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    send(sock , (char*)&*msg_req , sizeof(*msg_req) , 0 );
    valread = read( sock , (char*)&msg_res, sizeof(msg_res));
    std::cout << msg_res << std::endl;
    close(sock);
    return 0;


}