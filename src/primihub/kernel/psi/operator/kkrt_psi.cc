// "Copyright [2023] <PrimiHub>"
#include "src/primihub/kernel/psi/operator/kkrt_psi.h"
#include <utility>
#include <algorithm>

#include "cryptoTools/Network/IOService.h"
#include "cryptoTools/Common/config.h"
#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Crypto/RandomOracle.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "cryptoTools/Common/Timer.h"

#include "src/primihub/util/network/message_interface.h"
#include "libPSI/PSI/Kkrt/KkrtPsiSender.h"
#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtReceiver.h"
#include "libOTe/NChooseOne/Kkrt/KkrtNcoOtSender.h"
#include "libOTe/NChooseOne/NcoOtExt.h"

#include "src/primihub/util/endian_util.h"
#include "src/primihub/common/value_check_util.h"
#include "src/primihub/util/util.h"

namespace primihub::psi {
retcode KkrtPsiOperator::OnExecute(const std::vector<std::string>& input,
                                   std::vector<std::string>* result) {
  if (input.empty()) {
    LOG(ERROR) << "no data is set for kkrt psi";
    return retcode::FAIL;
  }
  oc::IOService ios;
  auto msg_interface = BuildChannelInterface();
  if (msg_interface == nullptr) {
    LOG(ERROR) << "BuildChannelInterface failed";
    return retcode::FAIL;
  }
  oc::Channel chl(ios, msg_interface.release());
  auto ret{retcode::SUCCESS};
  if (RoleValidation::IsClient(PartyName())) {
    std::vector<uint64_t> result_index;
    ret = KkrtRecv(chl, input, &result_index);
    this->GetResult(input, result_index, result);
  } else {
    ret = KkrtSend(chl, input);
  }
  return ret;
}

auto KkrtPsiOperator::BuildChannelInterface() ->
    std::unique_ptr<TaskMessagePassInterface> {
//
  std::string peer_party_name;
  if (RoleValidation::IsClient(PartyName())) {
    peer_party_name = PARTY_SERVER;
  } else if (RoleValidation::IsServer(PartyName())) {
    peer_party_name = PARTY_CLIENT;
  } else {
    LOG(ERROR) << "Invalid Party Name for KKrt Psi: " << this->PartyName()
               << " expected party name: [" << PARTY_CLIENT << ","
               << PARTY_SERVER <<"]";
    return nullptr;
  }
  auto it = options_.party_info.find(peer_party_name);
  if (it == options_.party_info.end()) {
    LOG(ERROR) << "get peer access info failed for: " << this->PartyName()
               << " peer party_name: " << peer_party_name;
    return nullptr;
  }
  auto& peer_node = it->second;
  auto link_ctx = options_.link_ctx_ref;
  auto send_channel = link_ctx->getChannel(peer_node);
  // get proxy channel
  auto recv_channel = link_ctx->getChannel(options_.proxy_node);
  // The 'osuCrypto::Channel' will consider it to be a unique_ptr and will
  // reset the unique_ptr, so the 'osuCrypto::Channel' will delete it.
  auto msg_interface = std::make_unique<TaskMessagePassInterface>(
      this->PartyName(), peer_party_name, link_ctx, send_channel, recv_channel);
  return msg_interface;
}

retcode KkrtPsiOperator::KkrtRecv(oc::Channel& chl,
                                  const std::vector<std::string>& input,
                                  std::vector<uint64_t>* result_index) {
  u8 dummy[1];
  // oc::PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
  oc::PRNG prng(oc::block(time(nullptr), time(nullptr)));
  u64 sendSize;
  // u64 recvSize = 10;
  u64 recvSize = input.size();

  std::vector<u64> data{recvSize};
  chl.asyncSend(std::move(data));
  std::vector<u64> dest;
  chl.recv(dest);

  // LOG(INFO) << "send size:" << dest[0];
  sendSize = dest[0];
  std::vector<oc::block> sendSet(sendSize), recvSet(recvSize);
  SCopedTimer timer;
  HashDataParallel(input, &recvSet);
  auto time_cost = timer.timeElapse();
  VLOG(5) << "encrypt data cost time(ms): " << time_cost;
  oc::KkrtNcoOtReceiver otRecv;
  oc::KkrtPsiReceiver recvPSIs;
  // LOG(INFO) << "client step 1";

  // recvPSIs.setTimer(gTimer);
  chl.recv(dummy, 1);
  // LOG(INFO) << "client step 2";
  // gTimer.reset();
  chl.asyncSend(dummy, 1);
  // LOG(INFO) << "client step 3";
  // Timer timer;
  // auto start = timer.setTimePoint("start");
  auto start_init_receiver = timer.timeElapse();
  recvPSIs.init(sendSize, recvSize, 40, chl,
                otRecv, prng.get<oc::block>());
  auto end_init_receiver = timer.timeElapse();
  auto init_receiver_cost = end_init_receiver - start_init_receiver;
  VLOG(5) << "init psi receiver cost(ms): " << init_receiver_cost;
  // LOG(INFO) << "client step 4";
  // auto mid = timer.setTimePoint("init");
  auto start_psi_protocol = timer.timeElapse();
  recvPSIs.sendInput(recvSet, chl);
  auto end_psi_protocol = timer.timeElapse();
  auto psi_protocol_time_cost = end_psi_protocol - start_psi_protocol;
  VLOG(5) << "execute psi protocol cost(ms): " << psi_protocol_time_cost;
  // LOG(INFO) << "client step 5";
  // auto end = timer.setTimePoint("done");

  // GetIntsection index
  auto pos_start = timer.timeElapse();
  auto& intersection = recvPSIs.mIntersection;
  *result_index = std::move(intersection);
  auto pos_end = timer.timeElapse();
  auto pos_time_cost = pos_end - pos_start;
  VLOG(5) << "GetIntsection index cost(ms): " << pos_time_cost;

  return retcode::SUCCESS;
}

retcode KkrtPsiOperator::KkrtSend(oc::Channel& chl,
                                  const std::vector<std::string>& input) {
  u8 dummy[1];
  // osuCrypto::PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
  oc::PRNG prng(oc::block(time(nullptr), time(nullptr)));
  u64 sendSize = input.size();
  u64 recvSize;

  std::vector<u64> data{sendSize};
  chl.asyncSend(std::move(data));
  std::vector<u64> dest;
  chl.recv(dest);

  // LOG(INFO) << "recv size:" << dest[0];
  recvSize = dest[0];
  SCopedTimer timer;
  std::vector<oc::block> set(sendSize);
  HashDataParallel(input, &set);
  auto time_cost = timer.timeElapse();
  VLOG(5) << "encrypt data cost time(ms): " << time_cost;

  oc::KkrtNcoOtSender otSend;
  oc::KkrtPsiSender sendPSIs;
  // LOG(INFO) << "server step 1";

  sendPSIs.setTimer(oc::gTimer);
  chl.asyncSend(dummy, 1);
  // LOG(INFO) << "server step 2";
  chl.recv(dummy, 1);
  // LOG(INFO) << "server step 3";
  auto start_init_sender = timer.timeElapse();
  sendPSIs.init(sendSize, recvSize, 40, chl,
                otSend, prng.get<oc::block>());
  auto end_init_sender = timer.timeElapse();
  auto init_sender_cost = end_init_sender - start_init_sender;
  VLOG(5) << "init psi sender cost(ms): " << init_sender_cost;
  // LOG(INFO) << "server step 4";
  auto start_psi_protocol = timer.timeElapse();
  sendPSIs.sendInput(set, chl);
  // LOG(INFO) << "server step 5";
  auto end_psi_protocol = timer.timeElapse();
  auto psi_protocol_time_cost = end_psi_protocol - start_psi_protocol;
  VLOG(5) << "execute psi protocol cost(ms): " << psi_protocol_time_cost;
  u64 dataSent = chl.getTotalDataSent();
  // LOG(INFO) << "server step 6";

  chl.resetStats();
  // LOG(INFO) << "server step 7";
  return retcode::SUCCESS;
}

retcode KkrtPsiOperator::HashDataParallel(const std::vector<std::string>& input,
                                          std::vector<oc::block>* result_ptr) {
  if (result_ptr->size() != input.size()) {
    result_ptr->resize(input.size());
  }
  auto& result = *result_ptr;
  uint64_t data_size = input.size();
  uint64_t MAX_BLOCK_NUM = 10000000;  // 1000W
  uint64_t block_num = data_size / MAX_BLOCK_NUM;
  if (data_size % MAX_BLOCK_NUM) {
    block_num++;
  }
  if (block_num == 1) {
    u8 block_size = sizeof(oc::block);
    oc::RandomOracle sha1(block_size);
    u8 hash_dest[block_size];                                       // NOLINT
    for (u64 i = 0; i < data_size; ++i) {
      sha1.Update((u8 *)input[i].data(), input[i].size());          // NOLINT
      sha1.Final((u8 *)hash_dest);                                  // NOLINT
      result[i] = oc::toBlock(hash_dest);
      sha1.Reset();
    }
  } else {
    std::vector<std::future<void>> futs;
    for (uint64_t i = 0; i < block_num; i++) {
      uint64_t start_i = i * MAX_BLOCK_NUM;
      uint64_t end_i = std::min((i+1) * MAX_BLOCK_NUM, data_size);
      futs.push_back(
        std::async(
          std::launch::async,
          [&, start_i, end_i]() {
            u8 block_size = sizeof(oc::block);
            oc::RandomOracle sha1(block_size);
            u8 hash_dest[block_size];                               // NOLINT
            for (u64 index = start_i; index < end_i; ++index) {
              sha1.Update((u8 *)input[index].data(),                // NOLINT
                          input[index].size());
              sha1.Final((u8 *)hash_dest);                          // NOLINT
              result[index] = oc::toBlock(hash_dest);
              sha1.Reset();
            }
          }));
    }
    for (auto&& fut : futs) {
      fut.get();
    }
  }
  return retcode::SUCCESS;
}
}  // namespace primihub::psi
