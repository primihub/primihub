#include "src/primihub/protocol/aby3/aby3_channel.h"

void Aby3Channel::SetupCommChannel(const std::string &peer_node_id,
                                   std::shared_ptr<network::IChannel> channel) {
  peer_node_ = peer_node_id;
  channel_ = channel;
}
