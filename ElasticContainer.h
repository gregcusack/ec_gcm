//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_ELASTICCONTAINER_H
#define EC_GCM_ELASTICCONTAINER_H
#include "Manager.h"
#include "Server.h"
#include "SubContainer.h"
#include "om.h"

namespace ec {
    class ElasticContainer {
    public:
        ElasticContainer(uint32_t _ec_id, ip4_addr _ip_address, std::vector<std::string>& agents_ips);

        //creates manager and server and connects them
        void build_ec_handler(uint16_t _port);

        Manager* get_manager();
        struct memory {
            memory(int64_t _max_mem) : memory_limit(_max_mem) {};
            memory() = default;
            //init to 30000 pages (that's what maziyar has it as)
            uint64_t memory_limit           =   30000;                   //-1 no limit, (in mbytes or pages tbd)
            uint64_t slice_size             =   5000; //pages
            friend std::ostream& operator<<(std::ostream& os, const memory& rhs) {
                os << "memory_limit: " << rhs.memory_limit;
                return os;
            }
        };

        struct cpu {
            cpu(uint64_t _period, int64_t _quota, uint64_t _slice_size)
                : period(_period), quota(_quota), slice_size(_slice_size) {};
            cpu() = default;
            uint64_t period             =   100000000;      //100 ms
            int64_t quota               =   period;             //-1: no limit, in ms
            uint64_t slice_size         =   10000000;       //10 ms
            friend std::ostream& operator<<(std::ostream& os, const cpu& rhs) {
                os  << "period: " << rhs.period << ", "
                    << "quota: " << rhs.quota << ", "
                    << "slice_size: " << rhs.slice_size;
                return os;
            }
        };

        uint32_t get_ec_id() { return ec_id; };

        void set_max_mem(int64_t _max_mem) { _mem.memory_limit = _max_mem; }
        void set_period(int64_t _period)  { _cpu.period = _period; }   //will need to update maanger too
        void set_quota(int64_t _quota) { _cpu.quota = _quota; }
        void set_slice_size(uint64_t _slice_size) { _cpu.slice_size = _slice_size; }


    private:
        //setup functions
        void create_manager();
        void create_server(uint16_t port);
        void connect_server_and_manager();


        uint32_t ec_id;
        ip4_addr ip_address;
        Manager *manager;
        Server *server;

        //uint32_t num_agents;
	std::vector<std::string> agents_ips;

        memory _mem;
        cpu _cpu;

    };
}



#endif //EC_GCM_ELASTICCONTAINER_H
