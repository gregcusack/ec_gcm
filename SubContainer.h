//
// Created by greg on 9/11/19.
//

#ifndef EC_GCM_CONTAINER_H
#define EC_GCM_CONTAINER_H

#include <cstdint>
#include <iostream>
#include <utility>
#include "types/types.h"
#include "stats/local/cpu_l.h"
#include "stats/local/mem_l.h"
#include "spdlog/spdlog.h"

namespace ec {
    /*!
     * @class cg (cgroup)
     * @brief individual cgroup running on each server
     */
    class SubContainer {
    public:
        SubContainer(uint32_t cgroup_id, uint32_t ip, int fd);     //from uint32_t
        SubContainer(uint32_t cgroup_id, uint32_t ip, int fd, uint64_t _quota, uint32_t _nr_throttled);
//        SubContainer(const SubContainer &other_sc) {std::cout << "copy constructor!" << std::endl; };
        ~SubContainer() = default;

        struct ContainerId {
            ContainerId(uint32_t _cgroup_id, ip4_addr _ip)
                : cgroup_id(_cgroup_id), server_ip(_ip) {};
            ContainerId(uint32_t _cgroup_id, std::string _ip)
                    : cgroup_id(_cgroup_id), server_ip(om::net::ip4_addr::from_string(std::move(_ip))) {};

            ContainerId() = default;
            uint32_t cgroup_id          = 0;
            ip4_addr server_ip;
            bool operator==(const ContainerId& other_) const;
            friend std::ostream& operator<<(std::ostream& os, const ContainerId& rhs) {
                os  << "cgroup_id: " << rhs.cgroup_id << ", "
                    << "server_ip: " << rhs.server_ip;// << ", "
                return os;
            };
        };

        ContainerId* get_c_id() {return &c_id;}
        [[nodiscard]] int get_fd() const { return fd; }
        
        void set_docker_id(std::string &docker_id) { _docker_id = docker_id; }
        std::string get_docker_id() { return _docker_id; }

        uint64_t get_quota() { return cpu.get_quota(); }
        uint32_t get_throttled() { return cpu.get_throttled(); }

        void set_quota(uint64_t _quota) { cpu.set_quota(_quota); }
        void set_throttled(uint32_t _throttled) { cpu.set_throttled(_throttled); }

        uint32_t get_throttle_increase(uint32_t _throttled) { return cpu.get_throttle_increase(_throttled); }
        uint32_t get_thr_incr_and_set_thr(uint32_t _throttled);

        [[nodiscard]] int get_counter() const { return counter; }
        void incr_counter() { counter++; }

        local::stats::cpu *get_cpu_stats() { return &cpu; }
        local::stats::mem *get_mem_stats() { return &mem; }

        bool get_set_quota_flag() { return cpu.get_set_quota_flag(); }
        void set_quota_flag(bool val) { cpu.set_set_quota_flag(val); }

        //Mem
        uint64_t get_mem_limit_in_pages() { return mem.get_mem_limit_in_pages(); }
        void set_mem_limit_in_pages(uint64_t _new_limit) { mem.set_mem_limit_in_pages(_new_limit); }
        void sc_incr_mem_limit(uint64_t _incr) { mem.incr_mem_limit(_incr); }
        void sc_decr_mem_limit(uint64_t _decr) { mem.decr_mem_limit(_decr); }

        [[nodiscard]] bool sc_inserted() const { return inserted; }
        void set_sc_inserted(bool _inserted) { inserted = _inserted; }

    private:
        ContainerId c_id;
        int fd;
        std::string _docker_id;
        bool inserted;

        local::stats::cpu cpu;
        local::stats::mem mem;

        int counter;


    };


}
//
namespace std {
    template<>
    struct hash<ec::SubContainer::ContainerId> {
        std::size_t operator()(ec::SubContainer::ContainerId const& p) const {
            auto h1 = std::hash<uint32_t>()(p.cgroup_id);
            auto h2 = std::hash<om::net::ip4_addr>()(p.server_ip);
            return h1 xor h2;// xor h3;
        }
    };
}




#endif //EC_GCM_CONTAINER_H
