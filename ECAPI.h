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
#include "cAdvisorSDK/include/cAdvisorFacade.h"
#define __FAILED__ -1

namespace ec {
    class ECAPI {
    using subcontainer_map = std::unordered_map<SubContainer::ContainerId, SubContainer *>;
    using subcontainer_agent_map = std::unordered_map<SubContainer::ContainerId, AgentClient*>;

    public:
//        ECAPI(uint32_t _ec_id, ip4_addr _ip_address, uint16_t _port, std::vector<Agent *> &_agents);
        ECAPI(){}
        ECAPI(uint32_t _ec_id);
        ~ECAPI();
        //creates _ec and server and connects them
        int create_ec(const std::string &app_name, const std::vector<std::string> &app_images, const std::vector<std::string> &pod_names, const std::string &gcm_ip);

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
        //TODO: this should be wrapped in ElasticContainer - shouldn't be able to access from Manager
        [[nodiscard]] const subcontainer_map  &get_subcontainers() const {return _ec->get_subcontainers(); }
        [[nodiscard]] const subcontainer_agent_map  &get_subcontainer_agents() const {return _ec->get_subcontainer_agents(); }

        const SubContainer &get_subcontainer(SubContainer::ContainerId &container_id) {return _ec->get_subcontainer(
                    container_id);}
        SubContainer *ec_get_sc_for_update(SubContainer::ContainerId &container_id) {return _ec->get_sc_for_update(
                    container_id);}

        //CPU
        //TODO: this has to be passed a specific sc id
        uint64_t sc_get_cpu_runtime_remaining() { return _ec->get_cpu_rt_remaining(); }
        uint64_t ec_get_cpu_runtime_remaining() { return _ec->get_cpu_rt_remaining(); }
        uint64_t ec_get_cpu_unallocated_rt() { return _ec->get_cpu_unallocated_rt(); }
        uint64_t ec_get_cpu_slice() { return _ec->get_cpu_slice(); }
        uint64_t ec_get_fair_cpu_share() { return _ec->get_fair_cpu_share(); }
        uint64_t ec_get_overrun() { return _ec->get_overrun(); }
        uint64_t ec_get_total_cpu() { return _ec->get_total_cpu(); }
        int64_t get_cpu_quota_in_us(const SubContainer::ContainerId &container_id);


        //MEM
        uint64_t ec_get_memory_available() { return _ec->get_memory_available(); }
        uint64_t ec_get_memory_slice() { return _ec->get_memory_slice(); }
        uint64_t get_memory_limit_in_bytes(const SubContainer::ContainerId &container_id);
        uint64_t get_memory_usage_in_bytes(const SubContainer::ContainerId &container_id);
        
        // Machine Stats
        uint64_t get_machine_free_memory(const SubContainer::ContainerId &container_id);

        //AGENTS
        //uint32_t get_num_agent_clients() { return _ec->get_num_agent_clients(); }
        // [[nodiscard]] const std::vector<AgentClient*> &get_agent_clients() const {return _ec->get_agent_clients(); }
        int64_t get_sc_quota(ec::SubContainer *sc);

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
        void ec_incr_unallocated_rt(uint64_t _incr) { _ec->incr_unallocated_rt(_incr); }
        void ec_decr_unallocated_rt(uint64_t _decr) { _ec->decr_unallocated_rt(_decr); }
        void ec_incr_total_cpu(uint64_t _incr) { _ec->incr_total_cpu(_incr); }
        void ec_decr_total_cpu(uint64_t _decr) { _ec->decr_total_cpu(_decr); }
        void ec_incr_overrun(uint64_t _incr) { _ec->incr_overrun(_incr); }
        void ec_decr_overrun(uint64_t _decr) { _ec->decr_overrun(_decr); }
        void ec_set_overrun(uint64_t _val) {_ec->set_overrun(_val); }
        void ec_set_unallocated_rt(uint64_t _val) {_ec->set_unallocated_rt(_val); }

        int64_t set_sc_quota(ec::SubContainer *sc, uint64_t _quota, uint32_t seq_number);

        //MEM
        void ec_resize_memory_max(int64_t _max_mem) { _ec->ec_resize_memory_max(_max_mem); }
        void ec_decrement_memory_available(uint64_t mem_to_reduce);
        uint64_t ec_set_memory_available(uint64_t mem) { return _ec->ec_set_memory_available(mem); }

        int64_t resize_memory_limit_in_bytes(ec::SubContainer::ContainerId container_idm, uint64_t new_mem_limit);

        /**
         *******************************************************
         * EVENTS
         * ALL CPU AND MEMORY EVENTS HANDLED HERE
         *******************************************************
         **/
        
        //TODO: implement these here in a class that inherits from manager
        int handle_add_cgroup_to_ec(const msg_t *req, msg_t *res, uint32_t ip, int fd);
        //CPU
        virtual int handle_cpu_usage_report(const msg_t *req, msg_t *res) = 0;
//        virtual int64_t set_sc_quota(SubContainer *sc, uint64_t _quota) = 0;
//        int handle_slice_req(const msg_t *req, msg_t *res, int clifd);

        //MEMORY
        virtual int handle_mem_req(const msg_t *req, msg_t *res, int clifd) = 0;
        virtual uint64_t handle_reclaim_memory(int client_fd) = 0;

        /**
         * HANDLERS
         */


    protected:
        std::mutex mtx;           
        uint32_t manager_id;
        ElasticContainer *_ec;
        std::mutex sendlock;

    };
}



#endif //EC_GCM_ECAPI_H
