//
// Created by Greg Cusack on 10/19/19.
//

#ifndef EC_GCM_AGENT_H
#define EC_GCM_AGENT_H

#include "om.h"
#include <iostream>

namespace ec {
    class Agent {
    public:
        Agent(std::string ip_addr_)
            : ip(om::net::ip4_addr::from_string(std::move(ip_addr_))),
            port(4445), sockfd(0) {}

        [[nodiscard]] om::net::ip4_addr get_ip() const { return ip; };
        [[nodiscard]] uint16_t get_port() const { return port; };
        [[nodiscard]] int get_sockfd() const { return sockfd; };

        void set_port(uint16_t _port) { port = _port; };
        void set_sockfd(int _sockfd) { sockfd = _sockfd; };

    private:
        om::net::ip4_addr ip;
        uint16_t port;
        int sockfd;

        friend std::ostream& operator<<(std::ostream& os, const Agent& rhs) {
            os << "ip: " << rhs.ip << ", port: " << rhs.port << ", sockfd: " << rhs.sockfd;// << std::endl;
            return os;
        }
    };
};


#endif //EC_GCM_AGENT_H
