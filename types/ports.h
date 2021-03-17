//
// Created by greg on 3/16/21.
//

#ifndef EC_GCM_PORTS_H
#define EC_GCM_PORTS_H

#include <cstdint>


namespace ec {
    struct ports_t {
        ports_t(const uint16_t _tcp, const uint16_t _udp) : tcp(_tcp), udp(_udp) {}

        uint16_t tcp;
        uint16_t udp;

        friend std::ostream& operator<<(std::ostream& os_, const ports_t& k) {
            return os_ << "ports_t: "
                       << k.tcp << ","
                       << k.udp;
        }

    };
}

#endif //EC_GCM_PORTS_H
