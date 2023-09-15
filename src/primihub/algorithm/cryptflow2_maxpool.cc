// Copyright [2021] <primihub.com>
#include "src/primihub/algorithm/cryptflow2_maxpool.h"
#include "src/primihub/data_store/factory.h"
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/util/cpu_check.h"
#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>
#include <glog/logging.h>
#include <omp.h>

using namespace std;
using namespace primihub::sci;
using namespace primihub::cryptflow2;
using namespace Eigen;
using arrow::Array;
using arrow::Table;

namespace primihub::cryptflow2 {

int party = 0;                // __party ID
int bitlength = 32;           // __bitlength of input
int num_threads = 1;          // thread_number
string address = "127.0.0.1"; // network __address
int port = 32000;             // network ports

} // namespace primihub::cryptflow2

MaxPoolExecutor::MaxPoolExecutor(
    PartyConfig &config, std::shared_ptr<DatasetService> dataset_service)
    : AlgorithmBase(dataset_service) {

  if (checkInstructionSupport("aes")) {
    LOG(ERROR) << "aes is required but not support in this platform.";
    LOG(ERROR) << "Dump cpu info:";
    PrintCPUInfo();

    throw std::runtime_error(
        "aes is required but not support in this platform.");
  } else {
    LOG(INFO) << "Current cpu support aes instruction.";
  }

  if (checkInstructionSupport("avx")) {
    LOG(ERROR) << "avx is required but not support in this platform.";
    LOG(ERROR) << "Dump cpu info:";
    PrintCPUInfo();

    throw std::runtime_error(
        "avx is required but not support in this platform.");
  } else {
    LOG(INFO) << "Current cpu support avx instruction.";
  }


  if (checkInstructionSupport("avx2")) {
    LOG(ERROR) << "avx2 is required but not support in this platform.";
    LOG(ERROR) << "Dump cpu info:";
    PrintCPUInfo();

    throw std::runtime_error(
        "avx2 is required but not support in this platform.");
  } else {
    LOG(INFO) << "Current cpu support avx2 instruction.";
  }

  if (checkInstructionSupport("sse4_1")) {
    LOG(ERROR) << "sse4.1 is required but not support in this platform.";
    LOG(ERROR) << "Dump cpu info:";
    PrintCPUInfo();

    throw std::runtime_error(
        "sse4.1 is required but not support in this platform.");
  } else {
    LOG(INFO) << "Current cpu support sse4.1 instruction.";
  }

  if (checkInstructionSupport("rdseed")) {
    LOG(ERROR) << "rdseed is required but not support in this platform.";
    LOG(ERROR) << "Dump cpu info:";
    PrintCPUInfo();

    throw std::runtime_error(
        "rdseed is required but not support in this platform.");
  } else {
    LOG(INFO) << "Current cpu support rdseed instruction.";
  }

  this->algorithm_name_ = "maxpool";
  // set the party id to be 1,2.
  uint16_t party_index = 0;
  auto& node_map = config.node_map;
  for (auto iter = node_map.begin(); iter != node_map.end(); iter++) {
    rpc::Node &node = iter->second;
    auto *vm = node.mutable_vm();
    auto target = vm->begin();
    target->set_party_id(party_index++);
    LOG(INFO) << "Assign party id " << target->party_id() << " to node "
              << node.node_id();
  }

  node_id = config.node_id;
}

int MaxPoolExecutor::loadParams(primihub::rpc::Task &task) {
  auto ret = this->ExtractProxyNode(task, &this->proxy_node_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "extract proxy node failed";
    return -1;
  }
  std::string party_name = task.party_name();
  const auto& party_access_info = task.party_access_info();
  auto it = party_access_info.find(party_name);
  if (it == party_access_info.end()) {
    LOG(ERROR) << "Can't find config with party_name: " << party_name;
    return -1;
  }
  const auto& node = it->second;
  // const rpc::Node node = iter->second;
  const rpc::VirtualMachine &vm = node.vm(0);
  auto param_map = task.params().param_map();

  const auto& party_datasets = task.party_datasets();
  auto dataset_iter = party_datasets.find(party_name);
  if (dataset_iter == party_datasets.end()) {
    LOG(ERROR) << "node datasets set for party_name: " << party_name;
    return -1;
  }
  {
    const auto& dataset_map = dataset_iter->second.data();
    auto it = dataset_map.find(party_name);
    if (it == dataset_map.end()) {
      LOG(ERROR) << "node datasets set for party_name: " << party_name;
      return -1;
    }
    input_filepath_ = it->second;
  }
  __party = vm.party_id() + 1;

  rpc::EndPoint ep = vm.next();
  __address = ep.ip();
  __port = ep.port();

  LOG(INFO) << "Notice: node " << node_id << ", party id " << __party
            << ", host " << __address << ", port " << __port << ".";

  LOG(INFO) << "Input data " << input_filepath_ << ".";

  return 0;
}

int MaxPoolExecutor::loadDataset() {
  /************ Generate Test Data ************/
  /********************************************/
  mask_l;
  if (__bitlength == 64)
    mask_l = -1;
  else
    mask_l = (1ULL << __bitlength) - 1;
  // generate fake data
  // PRG128 prg;
  // x = new uint64_t[this->num_rows * this->num_cols];
  // z = new uint64_t[this->num_rows];
  // prg.random_data(x, sizeof(uint64_t) * this->num_rows * this->num_cols);
  // for (int i = 0; i < this->num_rows * this->num_cols; i++)
  // {
  //   x[i] = x[i] & mask_l;
  // }

  // read data from csv
  std::string dataset_id = input_filepath_;
  auto driver = this->datasetService()->getDriver(dataset_id);
  auto cursor = driver->read();
  std::shared_ptr<Dataset> ds = cursor->read();
  std::shared_ptr<Table> table = std::get<std::shared_ptr<Table>>(ds->data);
  // Label column.
  bool errors = false;
  num_cols = table->num_columns();

  // 'array' include values in a column of csv file.
  auto array = table->column(num_cols - 1)->chunk(0);
  num_rows = array->length();

  // Force the same value count in every column.
  for (int i = 0; i < num_cols - 1; i++) {
    auto array = table->column(i)->chunk(0);
    if (array->length() != num_rows) {
      LOG(ERROR) << "row length doesn't match";
      errors = true;
      break;
    }
  }

  if (errors)
    return -1;

  x = new uint64_t[this->num_rows * this->num_cols];
  z = new uint64_t[this->num_rows];

  // x = new uint64_t[this->num_rows * this->num_cols];
  for (int i = 0; i < num_cols; i++) {
    auto array =
        std::static_pointer_cast<arrow::Int64Array>(table->column(i)->chunk(0));
    for (int64_t j = 0; j < array->length(); j++)
      x[j * num_cols + i] = array->Value(j);
  }

  return 0;
}

int MaxPoolExecutor::initPartyComm() {
  /********** Setup IO and Base OTs ***********/
  /********************************************/
  for (int i = 0; i < __num_threads; i++) {
    iopackArr[i] = new IOPack(__party, __port + i, __address);
    if (i & 1) {
      otpackArr[i] = new OTPack(iopackArr[i], 3 - __party);
    } else {
      otpackArr[i] = new OTPack(iopackArr[i], __party);
    }
  }

  LOG(INFO) << "All Base OTs Done.";
  return 0;
}

int MaxPoolExecutor::execute() {
  auto start = clock_start();
  int chunk_size = num_rows / __num_threads;
  for (int i = 0; i < __num_threads; ++i) {
    int offset = i * chunk_size;
    int lnum_rows;
    if (i == (__num_threads - 1)) {
      lnum_rows = num_rows - offset;
    } else {
      lnum_rows = chunk_size;
    }
    ring_maxpool_thread(i, z + offset, x + offset * num_cols, lnum_rows,
                        num_cols);
  }

  long long t = time_from(start);

  /************** Verification ****************/
  /********************************************/

  switch (__party) {
  case primihub::sci::ALICE: {
    iopackArr[0]->io->send_data(x, sizeof(uint64_t) * num_rows * num_cols);
    iopackArr[0]->io->send_data(z, sizeof(uint64_t) * num_rows);
    break;
  }
  case primihub::sci::BOB: {
    uint64_t *xi = new uint64_t[num_rows * num_cols];
    uint64_t *zi = new uint64_t[num_rows];
    iopackArr[0]->io->recv_data(xi, sizeof(uint64_t) * num_rows * num_cols);
    iopackArr[0]->io->recv_data(zi, sizeof(uint64_t) * num_rows);

    for (int i = 0; i < num_rows; i++) {
      zi[i] = (zi[i] + z[i]) & mask_l;
      for (int c = 0; c < num_cols; c++) {
        xi[i * num_cols + c] =
            (xi[i * num_cols + c] + x[i * num_cols + c]) & mask_l;
      }
      uint64_t maxpool_output = xi[i * num_cols];
      for (int c = 1; c < num_cols; c++) {
        maxpool_output = ((maxpool_output - xi[i * num_cols + c]) & mask_l) >=
                                 (1ULL << (__bitlength - 1))
                             ? xi[i * num_cols + c]
                             : maxpool_output;
      }
      assert((zi[i] == maxpool_output) && "MaxPool output is incorrect");
    }
    LOG(INFO) << "Maxpool output:";
    cout << "[";
    for (int i = 0; i < num_rows; i++) {
      std::cout << zi[i] << ", ";
    }
    std::cout << "] \n";
    delete[] xi;
    delete[] zi;
    LOG(INFO) << "Maxpool Passed.";
    break;
  }
  }

  LOG(INFO) << "Number of Maxpool rows (num_cols=" << num_cols << ")/s:\t"
            << (double(num_rows) / t) * 1e6 << std::endl;
  LOG(INFO) << "Maxpool Time (bitlength=" << __bitlength << "; b=" << b << ")\t"
            << t << " mus" << endl;

  delete[] x;
  delete[] z;
}

int MaxPoolExecutor::finishPartyComm() {
  /******************* Cleanup ****************/
  /********************************************/

  for (int i = 0; i < __num_threads; i++) {
    delete iopackArr[i];
    delete otpackArr[i];
  }
  return 0;
}

void MaxPoolExecutor::ring_maxpool_thread(int tid, uint64_t *z, uint64_t *x,
                                          int lnum_rows, int lnum_cols) {
  MaxPoolProtocol<uint64_t> *maxpool_oracle;
  if (tid & 1) {
    maxpool_oracle = new MaxPoolProtocol<uint64_t>(
        3 - __party, RING, iopackArr[tid], __bitlength, b, 0, otpackArr[tid]);
  } else {
    maxpool_oracle = new MaxPoolProtocol<uint64_t>(
        __party, RING, iopackArr[tid], __bitlength, b, 0, otpackArr[tid]);
  }
  if (batch_size) {
    for (int j = 0; j < lnum_rows; j += batch_size) {
      if (batch_size <= lnum_rows - j) {
        maxpool_oracle->funcMaxMPC(batch_size, lnum_cols, x + j, z + j,
                                   nullptr);
      } else {
        maxpool_oracle->funcMaxMPC(lnum_rows - j, lnum_cols, x + j, z + j,
                                   nullptr);
      }
    }
  } else {
    maxpool_oracle->funcMaxMPC(lnum_rows, lnum_cols, x, z, nullptr);
  }

  delete maxpool_oracle;
}

int MaxPoolExecutor::saveModel(void) {
  // Do nothing.
  return 0;
}
