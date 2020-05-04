//
// Created by greg on 4/29/20.
//

#ifndef EC_GCM_DEPLOYEREXPORTSERVICEIMPL_H
#define EC_GCM_DEPLOYEREXPORTSERVICEIMPL_H

#include <grpc++/grpc++.h>
#include <unordered_map>
#include "../om.h"
#include <cassert>
#include <mutex>
#include <iostream>
#include <cstdint>
#include <utility>

#include "deploy.pb.h"
#include "deploy.grpc.pb.h"
#include "../SubContainer.h"
#include "../Agents/AgentClient.h"
#include "../ElasticContainer.h"

namespace ec {
    namespace rpc {
        class DeployerExportServiceImpl final : public DeployerExport::Service {
            using sc_ac_map_type = std::unordered_map<SubContainer::ContainerId, AgentClient*>;
        public:
            DeployerExportServiceImpl(ElasticContainer *_ec, std::condition_variable &_cv,
                    std::mutex &_cv_mtx)
                : success("thx"), fail("fail"), ec(_ec), cv(_cv), cv_mtx(_cv_mtx) {
                if(!_ec) {
                    std::cout << "_ec is null!" << std::endl;
                } else if(!ec) {
                    std::cout << "ec is null" << std::endl;
                }
                else {
                        std::cout << "ec_id in constrcutor: " << _ec->get_ec_id() << std::endl;
                }
            }

            grpc::Status ReportPodSpec(grpc::ServerContext* context,
                    const ExportPodSpec* pod, PodSpecReply* reply) override;

            const std::unordered_map<SubContainer::ContainerId, std::string>& getDeployedPods() { return deployedPods; }

            struct matchingThreadArgs {
                matchingThreadArgs(const SubContainer::ContainerId& _sc_id, std::string  _docker_id)
                    : sc_id(_sc_id), docker_id(std::move(_docker_id)) {}
                SubContainer::ContainerId sc_id;
                std::string docker_id = "";
            };

        private:
            std::unordered_map<SubContainer::ContainerId, std::string> deployedPods;
            int insertPodSpec(const ExportPodSpec* pod);
            static void setPodSpecReply(const ExportPodSpec* pod, PodSpecReply* reply, std::string &status);
            void spinUpDockerIdThread(const SubContainer::ContainerId& sc_id, const std::string& docker_id);
            void scIdToDockerIdMatcherThread(void* args);

            std::mutex mapLock, &cv_mtx;
            std::condition_variable &cv;
            const std::string success, fail;
            ElasticContainer *ec;






        };

    }
}


#endif //EC_GCM_DEPLOYEREXPORTSERVICEIMPL_H
