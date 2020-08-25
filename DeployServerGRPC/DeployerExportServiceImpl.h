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
//#include "../Manager.h"

namespace ec {
    namespace rpc {
        class DeployerExportServiceImpl final : public DeployerExport::Service {
            using sc_ac_map_type = std::unordered_map<SubContainer::ContainerId, AgentClient*>;
        public:
            DeployerExportServiceImpl(ElasticContainer *_ec, std::condition_variable &_cv,
                std::condition_variable &_cv_dock, std::mutex &_cv_mtx, std::mutex &_cv_mtx_dock, std::mutex &_sc_lock)
                : success("thx"), fail("fail"), ec(_ec), cv(_cv), cv_dock(_cv_dock), cv_mtx(_cv_mtx),
                  cv_mtx_dock(_cv_mtx_dock), sc_lock(_sc_lock) {}

            grpc::Status ReportPodSpec(grpc::ServerContext* context,
                    const ExportPodSpec* pod, PodSpecReply* reply) override;

            grpc::Status DeletePod(grpc::ServerContext* context,
                    const ExportDeletePod* pod, DeletePodReply* reply) override;


            const std::unordered_map<SubContainer::ContainerId, std::string>& getDeployedPods() { return deployedPods; }

            struct matchingThreadArgs {
                matchingThreadArgs(const SubContainer::ContainerId& _sc_id, std::string  _docker_id)
                    : sc_id(_sc_id), docker_id(std::move(_docker_id)) {}
                SubContainer::ContainerId sc_id;
                std::string docker_id = "";
            };

            grpc::Status ReportAppSpec(grpc::ServerContext *context, const ec::rpc::ExportAppSpec *appSpec,
                    ec::rpc::AppSpecReply *reply) override;

        private:
            std::unordered_map<SubContainer::ContainerId, std::string> deployedPods;
            std::unordered_map<std::string, SubContainer::ContainerId> dockerToSubContainer;
            int insertPodSpec(const ExportPodSpec* pod);
            static void setPodSpecReply(const ExportPodSpec* pod, PodSpecReply* reply, std::string &status);
            void spinUpDockerIdThread(const SubContainer::ContainerId& sc_id, const std::string& docker_id);
            void scIdToDockerIdMatcherThread(void* args);

            int deleteFromScAcMap(const SubContainer::ContainerId &sc_id);
            int deleteFromSubcontainersMap(const SubContainer::ContainerId &sc_id);
            int deleteFromDeployedPodsMap(const SubContainer::ContainerId &sc_id);
            int deleteFromDockerIdScMap(const std::string &docker_id);

            void setDeletePodReply(const ExportDeletePod* pod, DeletePodReply* reply, const std::string &status);

            SubContainer::ContainerId getScIdFromDockerId(const std::string &docker_id);

            std::mutex dep_pod_lock, dockId_sc_lock, &cv_mtx, &cv_mtx_dock, &sc_lock;
            std::condition_variable &cv, &cv_dock;
            const std::string success, fail;
            ElasticContainer *ec;






        };

    }
}


#endif //EC_GCM_DEPLOYEREXPORTSERVICEIMPL_H
