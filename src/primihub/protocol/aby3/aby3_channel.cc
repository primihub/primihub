#include "src/primihub/protocol/aby3/aby3_channel.h"

namespace primihub {
void Aby3Channel::SetupCommChannel(const std::string &peer_node_id,
                                   std::shared_ptr<network::IChannel> channel,
                                   GetRecvQueueFunc get_queue_func) {
  peer_node_ = peer_node_id;
  channel_ = channel;
  get_queue_func_ = get_queue_func;
}

// uint64_t Aby3Channel::_GetCounterValue(void) {
//   uint64_t counter_val = 0;
//   counter_mu_.lock();
//   counter_val = counter_++;
//   counter_mu_.unlock();
//   return counter_val;
// }
// 
// int Aby3Channel::_channelRecv(ThreadSafeQueue<std::string> &queue,
//                               char *recv_ptr, uint64_t recv_size) {
//   std::string recv_str;
//   queue.wait_and_pop(recv_str);
// 
//   if (recv_str.size() != recv_size) {
//     LOG(ERROR) << "Recv buffer size mismatch, expect " << recv_str.size()
//                << " bytes, gives " << recv_size << " bytes.";
//     return -1;
//   }
// 
//   memcpy(recv_ptr, recv_str.c_str(), recv_size);
//   return 0;
// }
} // namespace primihub
