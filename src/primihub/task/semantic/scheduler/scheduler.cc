#include "src/primihub/task/semantic/scheduler/scheduler.h"
#include <google/protobuf/text_format.h>

namespace primihub::task {
VMScheduler::VMScheduler() {
  InitLinkContext();
}

VMScheduler::VMScheduler(const std::string &node_id, bool singleton)
    :node_id_(node_id), singleton_(singleton) {
  InitLinkContext();
}

retcode VMScheduler::dispatch(const PushTaskRequest* task_request_ptr) {
  PushTaskRequest task_request;
  task_request.CopyFrom(*task_request_ptr);
  std::string str;
  google::protobuf::TextFormat::PrintToString(task_request, &str);
  VLOG(5) << "VMScheduler::dispatch: " << str;
  const auto& participate_node = task_request.task().party_access_info();
  std::vector<std::thread> thrds;
  std::vector<std::future<retcode>> result_fut;
  for (const auto& [role, node_list] : participate_node) {
    for (const auto& pb_node : node_list.node()) {
      Node dest_node;
      pbNode2Node(pb_node, &dest_node);
      int32_t rank = pb_node.rank();
      VLOG(2) << "Dispatch Task to party: " << dest_node.to_string() << " "
          << "role: " << role << " rank: " << rank;
      result_fut.emplace_back(
        std::async(
          std::launch::async,
          &VMScheduler::ScheduleTask,
          this,
          role,
          rank,
          dest_node,
          std::ref(task_request)));
    }
  }
  bool has_error{false};
  for (auto&& fut : result_fut) {
    auto ret = fut.get();
    if (ret != retcode::SUCCESS) {
      has_error = true;
    }
  }
  if (has_error) {
    LOG(ERROR) << "dispatch task has error";
  } else {
    LOG(ERROR) << "dispatch task success";
  }
}

void VMScheduler::parseNotifyServer(const PushTaskReply& reply) {
  for (const auto& node : reply.task_server()) {
    Node task_server_info(node.ip(), node.port(), node.use_tls(), node.role());
    VLOG(5) << "task_server_info: " << task_server_info.to_string();
    this->addTaskServer(std::move(task_server_info));
  }
}

void VMScheduler::addTaskServer(Node&& node_info) {
  std::lock_guard<std::mutex> lck(task_server_mtx);
  task_server_info.push_back(std::move(node_info));
}

void VMScheduler::addTaskServer(const Node& node_info) {
  std::lock_guard<std::mutex> lck(task_server_mtx);
  task_server_info.push_back(node_info);
}

void VMScheduler::initCertificate() {
  auto& server_config = primihub::ServerConfig::getInstance();
  auto& host_cfg = server_config.getServiceConfig();
  if (host_cfg.use_tls()) {
    link_ctx_->initCertificate(server_config.getCertificateConfig());
  }
}

Node& VMScheduler::getLocalNodeCfg() const {
  auto& server_config = primihub::ServerConfig::getInstance();
  return server_config.getServiceConfig();
}

void VMScheduler::InitLinkContext() {
  link_ctx_ = primihub::network::LinkFactory::createLinkContext(primihub::network::LinkMode::GRPC);
  initCertificate();
}

retcode VMScheduler::ScheduleTask(const std::string& role,
    const int32_t rank, const Node dest_node, const PushTaskRequest& request) {
  VLOG(5) << "begin schedule task to party: " << role;
  SET_THREAD_NAME("VMScheduler");
  PushTaskReply reply;
  PushTaskRequest send_request;
  send_request.CopyFrom(request);
  auto task_ptr = send_request.mutable_task();
  task_ptr->set_role(role);
  task_ptr->set_rank(rank);
  // fill scheduler info
  {
    auto party_access_info_ptr = task_ptr->mutable_party_access_info();
    auto& local_node = getLocalNodeCfg();
    rpc::Node scheduler_node;
    node2PbNode(local_node, &scheduler_node);
    auto& schduler_node_list = (*party_access_info_ptr)[SCHEDULER_NODE];
    auto node_item = schduler_node_list.add_node();
    *node_item = std::move(scheduler_node);
  }
  // send request
  std::string dest_node_address = dest_node.to_string();
  LOG(INFO) << "dest node " << dest_node_address;
  auto channel = this->getLinkContext()->getChannel(dest_node);
  auto ret = channel->executeTask(send_request, &reply);
  if (ret == retcode::SUCCESS) {
    VLOG(5) << "send executeTask to : " << dest_node_address << " reply success";
  } else {
    LOG(ERROR) << "send executeTask to : " << dest_node_address << " reply failed";
    return retcode::FAIL;
  }
  parseNotifyServer(reply);
  return retcode::SUCCESS;
}

}  // namespace primihub::task