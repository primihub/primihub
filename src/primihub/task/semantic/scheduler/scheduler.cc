#include "src/primihub/task/semantic/scheduler/scheduler.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"

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
  if (VLOG_IS_ON(5)) {
    std::string str;
    str = proto::util::TaskRequestToString(*task_request_ptr);
    VLOG(5) << "VMScheduler::dispatch: " << str;
  }
  const auto& task_info = task_request_ptr->task().task_info();
  auto TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
  const auto& participate_node = task_request.task().party_access_info();
  std::vector<std::thread> thrds;
  std::vector<std::future<retcode>> result_fut;
  for (const auto& [party_name, node] : participate_node) {
    this->error_msg_.insert({party_name, ""});
  }
  for (const auto& [party_name, pb_node] : participate_node) {
    Node dest_node;
    pbNode2Node(pb_node, &dest_node);
    VLOG(2) << TASK_INFO_STR
        << "Dispatch Task to party: " << dest_node.to_string() << " "
        << "party_name: " << party_name;
    result_fut.emplace_back(
      std::async(
        std::launch::async,
        &VMScheduler::ScheduleTask,
        this,
        party_name,
        dest_node,
        std::ref(task_request)));
  }
  for (auto&& fut : result_fut) {
    auto ret = fut.get();
  }
  if (has_error()) {
    LOG(ERROR) << TASK_INFO_STR << "dispatch task has error";
    return retcode::FAIL;
  } else {
    VLOG(2) << TASK_INFO_STR << "dispatch task success";
    return retcode::SUCCESS;
  }
}

void VMScheduler::parseNotifyServer(const PushTaskReply& reply) {
  const auto& task_info = reply.task_info();
  auto TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
  for (const auto& node : reply.task_server()) {
    Node task_server_info(node.ip(), node.port(), node.use_tls(), node.role());
    VLOG(5) << TASK_INFO_STR
        << "task_server_info: " << task_server_info.to_string();
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

retcode VMScheduler::AddSchedulerNode(rpc::Task* task) {
  auto auxiliary_server_ptr = task->mutable_auxiliary_server();
  auto& local_node = getLocalNodeCfg();
  rpc::Node node;
  node2PbNode(local_node, &node);
  auto& scheduler_node = (*auxiliary_server_ptr)[SCHEDULER_NODE];
  scheduler_node = std::move(node);
  return retcode::SUCCESS;
}

Node& VMScheduler::getLocalNodeCfg() const {
  auto& server_config = primihub::ServerConfig::getInstance();
  return server_config.PublicServiceConfig();
}

void VMScheduler::InitLinkContext() {
  auto link_mode = primihub::network::LinkMode::GRPC;
  link_ctx_ = primihub::network::LinkFactory::createLinkContext(link_mode);
  initCertificate();
}

retcode VMScheduler::ScheduleTask(const std::string& party_name,
    const Node dest_node, const PushTaskRequest& request) {
  const auto& task_info = request.task().task_info();
  auto TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
  VLOG(5) << TASK_INFO_STR  << "begin schedule task to party: " << party_name;
  SET_THREAD_NAME("VMScheduler");
  PushTaskReply reply;
  PushTaskRequest send_request;
  send_request.CopyFrom(request);
  auto task_ptr = send_request.mutable_task();
  task_ptr->set_party_name(party_name);
  // fill scheduler info
  AddSchedulerNode(task_ptr);
  // send request
  std::string dest_node_address = dest_node.to_string();
  LOG(INFO) << TASK_INFO_STR << "dest node " << dest_node_address;
  auto channel = this->getLinkContext()->getChannel(dest_node);
  auto ret = channel->executeTask(send_request, &reply);
  if (ret == retcode::SUCCESS) {
    VLOG(5) << TASK_INFO_STR
        << "send executeTask to : " << dest_node_address << " reply success";
  } else {
    set_error();
    LOG(ERROR) << TASK_INFO_STR
        << "send executeTask to : " << dest_node_address << " reply failed";
    return retcode::FAIL;
  }
  parseNotifyServer(reply);
  return retcode::SUCCESS;
}

}  // namespace primihub::task