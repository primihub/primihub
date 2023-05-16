
#ifndef SRC_PRIMIHUB_PROTOCOL_ABY3_SHARE_H_
#define SRC_PRIMIHUB_PROTOCOL_ABY3_SHARE_H_

#include <glog/logging.h>
#include <glog/stl_logging.h>

#include "src/primihub/common/type/type.h"
#include "src/primihub/common/type/fixed_point.h"
#include "src/primihub/protocol/aby3/rand_ring_buffer.h"
#include "src/primihub/util/crypto/prng.h"
#include "function2/function2.hpp"

namespace primihub {


// temporary virtual party, it has role.
class Party {
 public:
  Party(const int role, block curr_seed, Channel& prev, Channel& next)
      : _role(role), _currSeed(curr_seed), _prev(prev), _next(next) {
    next.asyncSendCopy(_currSeed);
    prev.recv(_prevSeed);
    aby3Rand.init(_prevSeed, _currSeed);
    LOG(INFO) << "role : " << _role
        << " current seed :" << _currSeed << ", prev seed :" << _prevSeed << std::endl;
  }
  inline Channel& getPrev() { return _prev; }
  inline Channel& getNext() { return _next; }

  i64 getShare() {
    return aby3Rand.getShare();
  }

  int getRole() {
    return _role;
  }

  inline const int& getRole() const { return _role; }

  int _role;
  block _prevSeed;
  block _currSeed;
  ABY3RandBuffer aby3Rand;
  Channel& _prev, _next;
};

enum class RefType {
  LOCAL,
  IN,
  OUT
};

enum class DataState {
  Nil,
  Ready,
  Synched
};

using StateFun = fu2::unique_function<void()>;

template<class T>
class RefData {
 public:
  RefData() {}
  RefData(const T d, RefType ref_type) {
    _data = d;
    // _comm = comm;
    _ref_type = ref_type;
  }

  RefData& operator+(const RefData& r) const;
  RefData operator*(const RefData& r) const;
  RefData& operator=(const RefData& r);

  void sync() {
    if (_ref_type == RefType::LOCAL) {
      ;
    }
    else if (_ref_type == RefType::IN) {
      // _comm.recv(_data);
    } else if (_ref_type == RefType::OUT) {
      // _comm.send(_data);
    } else {
      // TODO error handling
    }
  }

  // Channel& _comm;
  RefType _ref_type;
  T _data;
};


template<class T>
RefData<T>& RefData<T>::operator+(const RefData<T>& r) const {
  RefData<T> rs;
  rs._data = _data + r._data;
  return rs;
}

template<class T>
RefData<T> RefData<T>::operator*(const RefData<T>& r) const {
  RefData<T> rs;
  rs._data = _data * r._data;
  return rs;
}

template<class T>
RefData<T>& RefData<T>::operator=(const RefData<T>& r) {
  _data = r._data;
  return *this;
}


class Share {

};


template<class T>
class ABY3Share : public Share {
 public:
  ABY3Share(Party& party)  : _party(party) {}
  ABY3Share(Party& party, const T curr) : _party(party) {
    i64 random = party.getShare();
    i64 sum_value = curr + random;
    LOG(INFO) << "party role : " << party.getRole()
        << " generate random : " << random << ", sum_value : " << sum_value << std::endl;

    _prev = RefData<T>(0, RefType::IN);
    _curr = RefData<T>(curr, RefType::OUT);
    LOG(INFO) << "ABY3Share, _curr :" << _curr._data << std::endl;

    block curr_block = toBlock(_curr._data);
    _party._next.send(curr_block);

    block prev_block = toBlock(static_cast<uint64_t>(0));
    _party._prev.recv(prev_block);

    _prev._data = prev_block.as<T>()[0];
  }

  // ABY3Share(const T curr, Channel& prev, Channel& next) : _prev_chl(prev), _next_chl(next) {
  //   _prev = RefData<T>(0, prev, RefType::IN);
  //   _curr = RefData<T>(curr, next, RefType::OUT);
  //   LOG(INFO) << "ABY3Share, _curr :" << _curr._data << std::endl;

  //   block curr_block = toBlock(_curr._data);
  //   _next_chl.send(curr_block);

  //   block prev_block = toBlock(0ull);
  //   _prev_chl.recv(prev_block);

  //   _prev._data = prev_block.as<T>()[0];
  // }

  ABY3Share& operator+(const ABY3Share& r) const;
  ABY3Share operator*(const ABY3Share& r);
  ABY3Share& operator=(const ABY3Share& r);

  T reveal();

  T getValue() { return _curr._data; }

  RefData<T> _prev;
  RefData<T> _curr;
  Party& _party;
};

template<class T>
ABY3Share<T>& ABY3Share<T>::operator+(const ABY3Share<T>& r) const {
  ABY3Share<T> rs(_party);
  rs._prev = _prev + r._prev;
  LOG(INFO) << "operator +, _curr :" << _curr._data << ", r._curr : " << r._curr._data << std::endl;
  rs._curr = _curr + r._curr;
  LOG(INFO) << "operator +, _curr :" << _curr._data << ", r._curr : " << r._curr._data << std::endl;
  return rs;
}

template<class T>
ABY3Share<T> ABY3Share<T>::operator*(const ABY3Share<T>& r) {
  /* C[0] = A[0] * B[0]
                + A[0] * B[1]
                + A[1] * B[0]
                + mShareGen.getShare();
  */
  ABY3Share<T> rs(*this);
  _curr = _curr * r._curr + _curr * r._prev + _prev * r._curr;
  LOG(INFO) << "operator *, _curr :" << _curr._data << std::endl;
  // _curr.sync();
  // _prev.sync();
  return rs;
}

template<class T>
ABY3Share<T>& ABY3Share<T>::operator=(const ABY3Share<T>& r) {
  _curr = r._curr;
  _prev = r._prev;
  // _party = r._party;
  // _curr.sync();
  // _prev.sync();
  return *this;
}

template<class T>
T ABY3Share<T>::reveal() {
  block result = toBlock(_curr._data);
  block prev_block = toBlock(_prev._data);
  block curr_block = toBlock(_curr._data);
  _party._prev.send(curr_block);

  block next_block = toBlock(static_cast<uint64_t>(0));
  _party._next.recv(next_block);
  LOG(INFO) << "reveal curr_block : " << curr_block << ", prev_block : "
      << prev_block << ", next_block: " << next_block << std::endl;
  result = curr_block + next_block + prev_block;
  return result.as<T>()[0];
}

template<class T>
class ABYShare {
 public:
  ABYShare() = default;
};

template<class T>
class ABY20Share {
 public:
  ABY20Share() = default;
};


class SInt {
 public:
  SInt(ABY3Share<i64>& share) : _share(share) {};
  SInt& operator+(const SInt& r);
  SInt operator*(const SInt& r) const;
  SInt& operator=(const SInt& r);

  i64 reveal() {
    return _share.reveal();
  }

  ABY3Share<i64>& _share;
};

SInt& SInt::operator+(const SInt& r) {
  ABY3Share<i64> s = _share + r._share;
  SInt rs(s);
  return rs;
}

SInt SInt::operator*(const SInt& r) const {
  ABY3Share<i64> share(_share._party, 0);
  SInt rs(share);
  rs._share = _share * r._share;
  return rs;
}

SInt& SInt::operator=(const SInt& r) {
  _share = r._share;
  return *this;
}

// infrustruture, ip + port
class SYS_Node {
 public:
  SYS_Node() {}
  void init(const std::string& node_id, const std::string& ip, const int port) {
    _node_id = node_id;
    _ip = ip;
    _port = port;
  }

  inline const std::string& getIP() const { return _ip; }
  inline const int getPort() const { return _port; }
  inline const std::string& getId() const { return _node_id; }
  Channel& getChannel(const std::string& node_id) {
    auto it = _channel.find(node_id);

    if ( it == _channel.end() ) {
      std::cout << "on node : " << _node_id << ", can't find node_id : " << node_id << std::endl;
    }
    else {
        return it->second;
    }
  }

  void add_party(std::string party_id, Party& _party) {
    party_map.emplace(party_id, _party);
  }

  void build_connect(SYS_Node& node) {
    std::string name = _node_id + node.getId();
    Session session_c(ios, node.getIP() + ":" + std::to_string(node.getPort()), SessionMode::Client, name);
    _sessions.emplace(name, session_c);
    _channel.emplace(name, session_c.addChannel(name, name));
    LOG(INFO) << "client node :" << _node_id
        << " build connect to " << node.getId()
        << " on ip:" << node.getIP() << " and port: "
        << node.getPort() << " with name : " << name << std::endl;

    node.build_server(name);
  }

  void build_server(std::string name) {
    Session session_s(ios, _ip + ":" + std::to_string(_port), SessionMode::Server, name);
    _sessions.emplace(name, session_s);

    _channel.emplace(name, session_s.addChannel(name, name));
    LOG(INFO) << "node :" << _node_id << " initialize server on ip:"
        << _port << " and port: " << _port << " with name : " << name << std::endl;
  }

  std::unordered_map<std::string, Session&> _sessions;
  std::unordered_map<std::string, Channel> _channel;

 private:
  std::string _node_id;
  std::string _ip;
  int _port;
  IOService ios;
  std::unordered_map<std::string, Party&> party_map;
};

// multi node to construct a network
class Network {
 public:
  Network() {}
  void add_node(std::string node_id, SYS_Node& node) {
    node_map.emplace(node_id, node);
    for(auto it = node_map.begin(); it != node_map.end(); ++it) {
      if (iter->first == SCHEDULER_NODE) {
        continue;
      }
      if(node_id != it->first) {
        SYS_Node& node_iter = it->second;
        node.build_connect(node_iter);
        node_iter.build_connect(node);
      }
    }
  }

 private:
  std::unordered_map<std::string, SYS_Node&> node_map;
};


enum class ProtocolType {
  ABY3,
  ABY,
  ABY20
};

class DistributedOracle {
 public:
  DistributedOracle() {}
  DistributedOracle(ProtocolType protocol) : _protocol(protocol) {

    SYS_Node n0;
    n0.init("0", "127.0.0.1", 13120);
    SYS_Node n1;
    n1.init("1", "127.0.0.1", 13121);
    SYS_Node n2;
    n2.init("2", "127.0.0.1", 13122);

    _network.add_node("0", n0);
    _network.add_node("1", n1);
    _network.add_node("2", n2);

    // where receive task, start run ABY3 protocol
    if (_protocol == ProtocolType::ABY3) {
      auto routine0 = [&](int role) {
        Party p0(0, toBlock(10), n0.getChannel("20"), n0.getChannel("01"));
        n0.add_party("0", p0);

        sleep(1);
        ABY3Share<i64> sharea(p0, 10);
        SInt a(sharea);
        LOG(INFO) << "party :" << 0 << " reveal a :" << a.reveal() << std::endl;

        ABY3Share<i64> shareb(p0, 20);
        SInt b(shareb);
        SInt c = a + b;
        // LOG(INFO) << "party :" << 0 << " reveal a :" << a.reveal() << std::endl;
        LOG(INFO) << "party :" << 0 << " reveal c :" << c.reveal()
            << " reveal a :" << a.reveal() << " reveal b :" << b.reveal() << std::endl;
      };

      auto routine1 = [&](int role) {
        Party p1(1, toBlock(11), n1.getChannel("01"), n1.getChannel("12"));
        n1.add_party("1", p1);

        sleep(1);
        ABY3Share<i64> sharea(p1, 0);
        SInt a(sharea);
        LOG(INFO) << "party :" << 1 << " reveal a :" << a.reveal() << std::endl;

        ABY3Share<i64> shareb(p1, 0);
        SInt b(shareb);
        SInt c = a + b;
        // LOG(INFO) << "party :" << 1 << " reveal a :" << a.reveal() << std::endl;
        LOG(INFO) << "party :" << 1 << " reveal c :" << c.reveal()
            << " reveal a :" << a.reveal() << " reveal b :" << b.reveal() << std::endl;
      };

      auto routine2 = [&](int role) {
        Party p2(2, toBlock(12), n2.getChannel("12"), n2.getChannel("20"));
        n2.add_party("2", p2);

        sleep(1);
        ABY3Share<i64> sharea(p2, 0);
        SInt a(sharea);
        LOG(INFO) << "party :" << 2 << " reveal a :" << a.reveal() << std::endl;

        ABY3Share<i64> shareb(p2, 0);
        SInt b(shareb);
        SInt c = a + b;
        // LOG(INFO) << "party :" << 2 << " reveal a :" << a.reveal() << std::endl;
        LOG(INFO) << "party :" << 2 << " reveal c :" << c.reveal()
            << " reveal a :" << a.reveal() << " reveal b :" << b.reveal() << std::endl;
      };

      auto t0 = std::thread(routine0, 0);
      auto t1 = std::thread(routine1, 1);
      auto t2 = std::thread(routine2, 2);

      t0.join();
      t1.join();
      t2.join();

      LOG(INFO) << "ABY3 protocol initiate 3 parties" << std::endl;
    }
  }

 private:
  ProtocolType _protocol;
  Network _network;
};



} // end of namespace

#endif
