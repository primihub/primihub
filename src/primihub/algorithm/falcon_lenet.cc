#include "src/primihub/algorithm/falcon_lenet.h"
namespace primihub {
namespace falcon {
AESObject *aes_indep;
AESObject *aes_next;
AESObject *aes_prev;
Precompute PrecomputeObject;
int partyNum;
std::string Test_Input_Self_filepath;
std::string Test_Input_Next_filepath;
// For faster modular operations
extern smallType additionModPrime[PRIME_NUMBER][PRIME_NUMBER];
extern smallType subtractModPrime[PRIME_NUMBER][PRIME_NUMBER];
extern smallType multiplicationModPrime[PRIME_NUMBER][PRIME_NUMBER];

FalconLenetExecutor::FalconLenetExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {
  this->algorithm_name_ = "Falcon_Lenet_Train";

  node_id_ = config.node_id;
  auto& node_map = config.node_map;
  auto iter = node_map.find(node_id_);
  if (iter == node_map.end()) {
    std::stringstream ss;
    ss << "Can't find node " << node_id_ << " in node map.";
    throw std::runtime_error(ss.str());
  }

  local_id_ = iter->second.vm(0).party_id();

  {
    const rpc::VirtualMachine &vm = iter->second.vm(0);
    const rpc::EndPoint &ep0 = vm.next();
    const rpc::EndPoint &ep1 = vm.prev();
    listen_addrs_.emplace_back(std::make_pair(ep0.ip(), ep0.port()));
    listen_addrs_.emplace_back(std::make_pair(ep1.ip(), ep1.port()));
    // VLOG(3) << "Addr to listen: " << listen_addrs_[0].first << ":"
    //         << listen_addrs_[0].second << ", " << listen_addrs_[1].first <<
    //         ":"
    //         << listen_addrs_[1].second << ".";
  }

  {
    const rpc::VirtualMachine &vm = iter->second.vm(1);
    const rpc::EndPoint &ep0 = vm.next();
    const rpc::EndPoint &ep1 = vm.prev();
    connect_addrs_.emplace_back(std::make_pair(ep0.ip(), ep0.port()));
    connect_addrs_.emplace_back(std::make_pair(ep1.ip(), ep1.port()));
    // VLOG(3) << "Addr to connect: " << connect_addrs_[0].first << ":"
    //         << connect_addrs_[0].second << ", " << connect_addrs_[1].first
    //         << ":" << connect_addrs_[1].second << ".";
  }

  // Key when save model.
  std::stringstream ss;
  ss << config.job_id << "_" << config.task_id << "_party_" << local_id_
     << "_lr";
  model_name_ = ss.str();
}

int FalconLenetExecutor::loadParams(primihub::rpc::Task &task) {
  auto ret = this->ExtractProxyNode(task, &this->proxy_node_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "extract proxy node failed";
    return -1;
  }
  auto param_map = task.params().param_map();
  try {
    Test_Input_Self_path = param_map["Test_Input_Self"].value_string();
    Test_Input_Next_path = param_map["Test_Input_Next"].value_string();
    batch_size_ = param_map["BatchSize"].value_int32();
    num_iter_ = param_map["NumIters"].value_int32(); // iteration rounds
  } catch (std::exception &e) {
    LOG(ERROR) << "Failed to load params: " << e.what();
    return -1;
  }

  LOG(INFO) << "Training on MNIST using Lenet:\t BatchSize:" << batch_size_
            << ", Epoch  " << num_iter_ << ".";
  return 0;
}

int FalconLenetExecutor::loadDataset() {

  Test_Input_Self_filepath = Test_Input_Self_path;
  Test_Input_Next_filepath = Test_Input_Next_path;

  return 0;
}

int FalconLenetExecutor::initPartyComm(void) {
  /****************************** AES SETUP and SYNC
   * ******************************/
  partyNum = local_id_;
  if (local_id_ == PARTY_A) {
    aes_indep = new AESObject("data/falcon/key/keyA");
    aes_next = new AESObject("data/falcon/key/keyAB");
    aes_prev = new AESObject("data/falcon/key/keyAC");
  }

  if (local_id_ == PARTY_B) {
    aes_indep = new AESObject("data/falcon/key/keyB");
    aes_next = new AESObject("data/falcon/key/keyBC");
    aes_prev = new AESObject("data/falcon/key/keyAB");
  }

  if (local_id_ == PARTY_C) {
    aes_indep = new AESObject("data/falcon/key/keyC");
    aes_next = new AESObject("data/falcon/key/keyAC");
    aes_prev = new AESObject("data/falcon/key/keyBC");
  }

  initializeCommunication(listen_addrs_, connect_addrs_);
  synchronize(2000000);

  return 0;
}

int FalconLenetExecutor::finishPartyComm(void) {

  /****************************** CLEAN-UP ******************************/

  delete aes_indep;
  delete aes_next;
  delete aes_prev;
  delete config_lenet;
  delete net_lenet;
  deleteObjects();
  return 0;
}

int FalconLenetExecutor::execute() {
  /****************************** PREPROCESSING ******************************/
  /*for faster module multiply and addition*/
  bool PRELOADING = false;
  for (int i = 0; i < PRIME_NUMBER; ++i)
    for (int j = 0; j < PRIME_NUMBER; ++j) {
      additionModPrime[i][j] = ((i + j) % PRIME_NUMBER);
      subtractModPrime[i][j] = ((PRIME_NUMBER + i - j) % PRIME_NUMBER);
      multiplicationModPrime[i][j] = ((i * j) % PRIME_NUMBER);
    }

  /****************************** SELECT NETWORK ******************************/
  std::string network = "LeNet";
  std::string dataset = "MNIST";
  std::string security = "Semi-honest";

  config_lenet = new NeuralNetConfig(num_iter_);
  selectNetwork(network, dataset, security, config_lenet);
  config_lenet->checkNetwork();
  net_lenet = new NeuralNetwork(config_lenet);

  // Test config
  network += " preloaded";
  PRELOADING = true;
  LOG(INFO) << "preloading begin";
  preload_network(PRELOADING, network, net_lenet);
  start_m();
  // LOG(INFO) << "Training on MNIST using Lenet";

  // for (int i = 0; i < num_iter_; ++i) {
  //   readMiniBatch(net_lenet, "TRAINING");
  //   net_lenet->forward();
  //   net_lenet->backward();
  // }

  LOG(INFO) << "Test on MNIST using Lenet";
  network += " test";
  test(PRELOADING, network, net_lenet);

  end_m(network);
  printNetwork(net_lenet);

  LOG(INFO) << "----------------------------------------------" << endl;
  LOG(INFO) << "Run details: " << NUM_OF_PARTIES << "PC (P" << partyNum << "), "
            << NUM_ITERATIONS << " iterations, batch size " << MINI_BATCH_SIZE
            << endl
            << "Running " << security << " " << network << " on " << dataset
            << " dataset" << endl;
  LOG(INFO) << "----------------------------------------------" << endl << endl;

  LOG(INFO) << "Party " << local_id_ << " train finish.";
  return 0;
}
} // namespace falcon
} // namespace primihub
