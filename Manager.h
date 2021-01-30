//
// Created by Greg Cusack on 11/5/19.
//

#ifndef EC_GCM_MANAGER_H
#define EC_GCM_MANAGER_H

#include "ECAPI.h"
#include "Server.h"
#include "DeployServerGRPC/DeployerExportServiceImpl.h"
#include "Agents/AgentClientDB.h"
#include <cstdint>
#include <mutex>
#include <future>
#include <numeric>
#include <vector>
#include <fstream> //For HotOS Logging
#include <chrono>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)
#define byte_to_page(x) ceil((x)/4096)
#define page_to_byte(x) (x*4096)
//#define _SAFE_MARGIN_BYTES_ 81920*250 //20 * 250 pages = 20MB
#define _SAFE_MARGIN_BYTES_ 81920*1250 //20 * 1250 pages = 100MB
#define _MAX_CPU_LOSS_IN_NS_ 1000
#define _MAX_UNUSED_RT_IN_NS_ 5000000

namespace ec {
    class Manager : public ECAPI, public Server {
    public:
        //TODO: initialize ECAPI and SERVER here?
        Manager(int server_counts, ip4_addr gcm_ip, uint16_t server_port, std::vector<Agent *> &agents);

        int handle_cpu_usage_report(const msg_t *req, msg_t *res) override;

        int handle_mem_req(const msg_t *req, msg_t *res, int clifd) override;
        uint64_t handle_reclaim_memory(int client_fd) override;

        int handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd) override;
        void start(const std::string &app_name, const std::string &gcm_ip);
        virtual void run();
        int handle_add_cgroup_to_ec(const msg_t *req, msg_t *res, uint32_t ip, int fd) override;

	uint64_t reclaim(SubContainer::ContainerId containerId, SubContainer* subContainer);
        // Need to remove this when Agent code gets merged with the correct codebase version
        struct reclaim_msg {
            uint16_t cgroup_id;
            uint32_t is_mem;
            //...maybe it needs more things
        };

        /**
         * GRPC
         */
        void serveGrpcDeployExport();
        ec::rpc::DeployerExportServiceImpl* getGrpcServer() {return grpcServer; }


    private:
        int manager_id;
        std::mutex cpulock;
        std::mutex memlock;
        int64_t seq_number;

        int64_t cpuleak;

        ec::rpc::DeployerExportServiceImpl *grpcServer;
        std::string deploy_service_ip;

        void determine_mem_limit_for_new_pod(SubContainer *sc, int clifd);

        /* HOTOS LOGGING */
        std::unordered_map<SubContainer::ContainerId, std::ofstream*> hotos_logs;


    };
}


#endif //EC_GCM_MANAGER_H
