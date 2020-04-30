//
// Created by greg on 4/29/20.
//

#ifndef EC_GCM_DEPLOYEREXPORTSERVICE_H
#define EC_GCM_DEPLOYEREXPORTSERVICE_H

#include <grpc++/grpc++.h>
#include <unordered_map>
#include "../om.h"
#include <cassert>
#include <mutex>

#include "deploy.pb.h"
#include "deploy.grpc.pb.h"
#include "../SubContainer.h"

namespace ec {
    namespace rpc {
        class DeployerExportService final : public ec::rpc::DeployerExport::Service {

        public:
            DeployerExportService() : success("thx"), fail("fail") { }

            grpc::Status ReportPodSpec(grpc::ServerContext* context,
                    const ExportPodSpec* pod, PodSpecReply* reply) override {
                std::string status;
                int ret = insertPodSpec(pod);
                status = ret ? fail : success;
                setPodSpecReply(pod, reply, status);
                return grpc::Status::OK;
            }

        private:
            std::unordered_map<SubContainer::ContainerId, std::string> deployedPods;
            int insertPodSpec(const ExportPodSpec* pod);
            static void  setPodSpecReply(const ExportPodSpec* pod, PodSpecReply* reply, std::string &status);

            std::mutex mapLock;
            const std::string success, fail;


        };

    }
}


#endif //EC_GCM_DEPLOYEREXPORTSERVICE_H
