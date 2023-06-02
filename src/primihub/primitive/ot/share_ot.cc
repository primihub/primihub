
#include "src/primihub/primitive/ot/share_ot.h"

namespace primihub {
#ifdef MPC_SOCKET_CHANNEL
void SharedOT::send(Channel & chl, span<std::array<i64, 2>> m) {
#else
void SharedOT::send(MpcChannel &chl, span<std::array<i64, 2>> m) {
#endif
  if (mIdx == -1)
    throw RTE_LOC;

  std::vector<std::array<i64, 2>> msgs(m.size());
  mAes.ecbEncCounterMode(mIdx, msgs.size(),
                         reinterpret_cast<block*>(msgs[0].data()));
  mIdx += msgs.size();

  for (u64 i = 0; i < msgs.size(); ++i) {
    msgs[i][0] = msgs[i][0] ^ m[i][0];
    msgs[i][1] = msgs[i][1] ^ m[i][1];
  }

  chl.asyncSend(std::move(msgs));
}

#ifdef MPC_SOCKET_CHANNEL
void SharedOT::help(Channel & chl, const BitVector& choices) {
#else
void SharedOT::help(MpcChannel & chl, const BitVector& choices) {
#endif
  if (mIdx == -1)
    throw RTE_LOC;

  std::vector<i64> mc(choices.size());
  std::vector<std::array<i64, 2>> msgs(choices.size());

  mAes.ecbEncCounterMode(mIdx, msgs.size(),
                         reinterpret_cast<block*>(msgs[0].data()));
  mIdx += msgs.size();

  for (u64 i = 0; i < msgs.size(); ++i) {
    mc[i] = msgs[i][choices[i]];
  }

  chl.asyncSend(std::move(mc));
}

void SharedOT::setSeed(const block & seed, u64 seedIdx) {
  mAes.setKey(seed);
  mIdx = seedIdx;
}

#ifdef MPC_SOCKET_CHANNEL
void SharedOT::recv(Channel & sender, Channel & helper,
                    const BitVector & choices, span<i64> recvMsgs) {
#else
void SharedOT::recv(MpcChannel &sender, MpcChannel &helper,
                    const BitVector & choices, span<i64> recvMsgs) {
#endif
  std::vector<std::array<i64, 2>> msgs(choices.size());
  std::vector<i64> mc(choices.size());

  sender.recv(msgs);
  helper.recv(mc);

  for (u64 i = 0; i < msgs.size(); ++i) {
    recvMsgs[i] = msgs[i][choices[i]] ^ mc[i];
  }
}

#ifdef MPC_SOCKET_CHANNEL
std::future<void> SharedOT::asyncRecv(Channel& sender,
    Channel& helper,
    BitVector&& choices,
    span<i64> recvMsgs) {
#else
std::future<void> SharedOT::asyncRecv(MpcChannel& sender,
    MpcChannel& helper,
    BitVector && choices,
    span<i64> recvMsgs) {
#endif
  auto m = std::make_shared<
                std::tuple<std::vector<std::array<i64, 2>>,
                           std::vector<i64>,
                           std::promise<void>,
                           BitVector,
                           std::atomic<int>
                           >
                >();

  std::get<0>(*m).resize(choices.size());
  std::get<1>(*m).resize(choices.size());
  std::get<3>(*m) = std::forward<BitVector>(choices);
  std::get<4>(*m) = 2;

  auto d0 = std::get<0>(*m).data();
  auto d1 = std::get<1>(*m).data();
  auto ret = std::get<2>(*m).get_future();

  auto cb = [m, recvMsgs]() mutable {
    auto& a = std::get<4>(*m);
    if (--a == 0) {
      auto& sendMsg = std::get<0>(*m);
      auto& recvMsg = std::get<1>(*m);
      auto& choices = std::get<3>(*m);
      for (u64 i = 0; i < sendMsg.size(); ++i) {
        recvMsgs[i] = sendMsg[i][choices[i]] ^ recvMsg[i];
      }

      std::get<2>(*m).set_value();
    }
  };

  sender.asyncRecv(d0, choices.size(), cb);
  helper.asyncRecv(d1, choices.size(), cb);

  return ret;
}

}  // namespace primihub
