
// Copyright [2021] <primihub.com>
#include <string>
#include <vector>
#include <iomanip>

#include "src/primihub/util/crypto/prng.h"
#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/network/socket/commpkg.h"

namespace primihub {

std::vector<CommPkg> comms;

void get_comms() {
  IOService ios;
  auto chl01 = Session(ios, "127.0.0.1:13130", SessionMode::Server,
                       "01").addChannel();
  auto chl10 = Session(ios, "127.0.0.1:13130", SessionMode::Client,
                       "01").addChannel();
  auto chl02 = Session(ios, "127.0.0.1:13131", SessionMode::Server,
                       "02").addChannel();
  auto chl20 = Session(ios, "127.0.0.1:13131", SessionMode::Client,
                       "02").addChannel();
  auto chl12 = Session(ios, "127.0.0.1:13132", SessionMode::Server,
                       "12").addChannel();
  auto chl21 = Session(ios, "127.0.0.1:13132", SessionMode::Client,
                       "12").addChannel();

  comms.push_back({ chl02, chl01 });
  comms.push_back({ chl10, chl12 });
  comms.push_back({ chl21, chl20 });
}

}
