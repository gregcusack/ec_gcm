//
// Created by greg on 9/12/19.
//

#ifndef EC_GCM_ECAPI_H
#define EC_GCM_ECAPI_H

#include "ElasticContainer.h"
#include "types/msg.h"
#include "Agents/Agent.h"
#include "Agents/AgentClient.h"
#include "om.h"
#include <iostream>
#include <functional> //for std::hash
#include <string>

#include "jsonSDK/include/JSONFacade.h"
#include "deploySDK/include/DeployFacade.h"
#include "protoBufSDK/include/ProtoBufFacade.h"
#include "protoBufSDK/msg.pb.h"

#define __FAILED__ -1

namespace ec {
    class ECAPI {
    using subcontainer_map = std::unordered_map<SubContainer::ContainerId, SubContainer *>;
    public:
//        ECAPI(uint32_t _ec_id, ip4_addr _ip_address, uint16_t _port, std::vector<Agent *> &_agents);
        ECAPI(){}
        ECAPI(uint32_t _ec_id);
        ~ECAPI();
        //creates _ec and server and connects them
//        void build_manager_handler();
        int create_ec(std::string app_name, std::string app_image);
        void deploy_application(std::string app_name, std::vector<std::string> app_images);

        [[nodiscard]] const ElasticContainer& get_elastic_container() const;

        uint32_t get_manager_id() { return manager_id; };

        /**
         *******************************************************
         * READ TYPE API
         * ALL CPU AND MEMORY READ FUNCTIONS DECLARED HERE
         *******************************************************
         **/
        //MISC
        uint32_t get_ec_id() { return _ec->get_ec_id(); }
        [[nodiscard]] const subcontainer_map  &get_subcontainers() const {return _ec->get_subcontainers(); }
        const SubContainer &get_subcontainer(SubContainer::ContainerId &container_id) {return _ec->get_subcontainer(
                    container_id);}

        //CPU
        //TODO: this has to be passed a specific sc id
        uint64_t sc_get_cpu_runtime_remaining() { return _ec->get_rt_remaining(); }
        uint64_t ec_get_cpu_runtime_remaining() { return _ec->get_rt_remaining(); }

        //MEM
        uint64_t ec_get_memory_available() { return _ec->get_memory_available(); }
        uint64_t ec_get_memory_slice() { return _ec->get_memory_slice(); }
        uint64_t get_memory_limit_in_bytes(ec::SubContainer::ContainerId container_id);


        //AGENTS
        uint32_t get_num_agent_clients() { return _ec->get_num_agent_clients(); }
        [[nodiscard]] const std::vector<AgentClient*> &get_agent_clients() const {return _ec->get_agent_clients(); }

        /**
         *******************************************************
         * WRITE TYPE API
         * ALL CPU AND MEMORY WRITE FUNCTIONS DECLARED HERE
         *******************************************************
         **/

        //CPU
        void ec_resize_period(int64_t _period)  { _ec->set_ec_period(_period); }
        void ec_resize_quota(int64_t _quota) { _ec->set_quota(_quota); }
        void ec_resize_slice(uint64_t _slice_size) { _ec->set_slice_size(_slice_size); }
        uint64_t ec_refill_runtime() {return _ec->refill_runtime(); }

        //MEM
        void ec_resize_memory_max(int64_t _max_mem) { _ec->ec_resize_memory_max(_max_mem); }
        void ec_decrement_memory_available(uint64_t mem_to_reduce);
        uint64_t ec_set_memory_available(uint64_t mem) { return _ec->ec_set_memory_available(mem); }

        /**
         *******************************************************
         * EVENTS
         * ALL CPU AND MEMORY EVENTS HANDLED HERE
         *******************************************************
         **/

        //MISC
//        int handle_req(const char *buff_in, char *buff_out, uint32_t host_ip, int clifd);
        virtual int handle_req(const msg_t *req, msg_t *res, uint32_t host_ip, int clifd) = 0;


        //TODO: implement these here in a class that inherits from manager
        int handle_add_cgroup_to_ec(msg_t *res, uint32_t cgroup_id, uint32_t ip, int fd);
        //CPU
        virtual int handle_cpu_req(const msg_t *req, msg_t *res) = 0;
//        int handle_slice_req(const msg_t *req, msg_t *res, int clifd);

        //MEMORY
        virtual int handle_mem_req(const msg_t *req, msg_t *res, int clifd) = 0;
        virtual uint64_t handle_reclaim_memory(int client_fd) = 0;

        /**
         * HANDLERS
         */

    protected:
        uint32_t manager_id;
        std::vector<AgentClient *> agent_clients;
        ElasticContainer *_ec;

    };
}



#endif //EC_GCM_ECAPI_H
