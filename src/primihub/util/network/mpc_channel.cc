// "Copyright [2023] <Primihub>"

#include "src/primihub/util/network/mpc_channel.h"

namespace primihub {
void MpcChannel::SetupBaseChannel(const std::string &peer_party_name,
                                  std::shared_ptr<network::IChannel> channel) {
  peer_party_name_ = peer_party_name;
  channel_ = channel;

  LOG(INFO) << "Init channel to " << peer_party_name_ << ".";
}

int MpcChannel::_channelRecvNonBlock(ThreadSafeQueue<std::string> &queue,
                                     char *recv_ptr, uint64_t recv_size,
                                     const std::string &recv_key) {
  std::string recv_str;
  queue.wait_and_pop(recv_str);

  if (recv_str.size() != recv_size) {
    LOG(ERROR) << "Recv buffer size mismatch, expect " << recv_str.size()
               << " bytes, gives " << recv_size  << " bytes, recv key "
               << recv_key << ".";
    return -1;
  }

  memcpy(recv_ptr, recv_str.c_str(), recv_str.size());

  VLOG(5) << "Recv finish, recv key " << recv_key << ", recv size " << recv_size
          << ", recv queue 0x" << std::hex << reinterpret_cast<uint64_t>(&queue)
          << ".";

  return 0;
}

int MpcChannel::_channelRecvNonBlock(ThreadSafeQueue<std::string> &queue,
                                     char *recv_ptr, uint64_t recv_size,
                                     const std::string &recv_key,
                                     std::function<void()> done) {
  int ret = _channelRecvNonBlock(queue, recv_ptr, recv_size, recv_key);
  if (ret) {
    LOG(ERROR) << "Recv with key " << recv_key
               << " failed, no callback triggered.";
    return -1;
  }

  done();
  return 0;
}

std::string HexToBin(const std::string &strHex) {
  if (strHex.size() % 2 != 0) {
    return "";
  }

  std::string strBin;
  strBin.resize(strHex.size() / 2);
  for (size_t i = 0; i < strBin.size(); i++) {
    uint8_t cTemp = 0;
    for (size_t j = 0; j < 2; j++) {
      char cCur = strHex[2 * i + j];
      if (cCur >= '0' && cCur <= '9') {
        cTemp = (cTemp << 4) + (cCur - '0');
      } else if (cCur >= 'a' && cCur <= 'f') {
        cTemp = (cTemp << 4) + (cCur - 'a' + 10);
      } else if (cCur >= 'A' && cCur <= 'F') {
        cTemp = (cTemp << 4) + (cCur - 'A' + 10);
      } else {
        return "";
      }
    }
    strBin[i] = cTemp;
  }

  return strBin;
}

std::string BinToHex(const std::string_view &strBin, bool bIsUpper) {
  std::string strHex;
  strHex.resize(strBin.size() * 2);
  for (size_t i = 0; i < strBin.size(); i++) {
    uint8_t cTemp = strBin[i];
    for (size_t j = 0; j < 2; j++) {
      uint8_t cCur = (cTemp & 0x0f);
      if (cCur < 10) {
        cCur += '0';
      } else {
        cCur += ((bIsUpper ? 'A' : 'a') - 10);
      }
      strHex[2 * i + 1 - j] = cCur;
      cTemp >>= 4;
    }
  }

  return strHex;
}
}  // namespace primihub
