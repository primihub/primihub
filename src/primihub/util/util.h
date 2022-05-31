// Copyright [2021] <primihub.com>
#ifndef SRC_primihub_UTIL_UTIL_H_
#define SRC_primihub_UTIL_UTIL_H_

#include <glog/logging.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <list>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>

#include "src/primihub/protos/worker.grpc.pb.h"

namespace primihub {

using primihub::rpc::Node;

void str_split(const std::string& str, std::vector<std::string>* v,
               char delimiter = ':');
void peer_to_list(const std::vector<std::string>& peer,
                  std::vector<Node>* list);

void sort_peers(std::vector<std::string>* peers);


class IOService;

class Work {
 public:
  std::unique_ptr<boost::asio::io_service::work> mWork;
  std::string mReason;
  IOService& mIos;
  Work(IOService& ios, std::string reason);

  Work(Work&&) = default;
  ~Work();

  void reset();
};

/* makes the whole thing 256 bytes */
template<typename T, int StorageSize = 248>
class SBO_ptr {
 public:
  struct SBOInterface {
    virtual ~SBOInterface() {};

    // assumes dest is uninitialized and calls the 
    // placement new move constructor with this as 
    // dest destination.
    virtual void moveTo(SBO_ptr<T, StorageSize>& dest) = 0;
  };

  template<typename U>
  struct Impl : SBOInterface {
    template<typename... Args,
        typename Enabled =
        typename std::enable_if<
        std::is_constructible<U, Args...>::value
        >::type>
    Impl(Args&& ... args):mU(std::forward<Args>(args)...) {}

    void moveTo(SBO_ptr<T, StorageSize>& dest) override {
      Expects(dest.get() == nullptr);
      dest.New<U>(std::move(mU));
    }

    U mU;
  };

  using base_type = T;
  using Storage = typename std::aligned_storage<StorageSize>::type;

  template<typename U>
  using Impl_type = Impl<U>;

  T* mData = nullptr;
  Storage mStorage;

  SBO_ptr() = default;
  SBO_ptr(const SBO_ptr<T>&) = delete;

  SBO_ptr(SBO_ptr<T>&& m) {
    *this = std::forward<SBO_ptr<T>>(m);
  }

  ~SBO_ptr() {
    destruct();
  }

  SBO_ptr<T, StorageSize>& operator=(SBO_ptr<T>&& m) {
    destruct();

    if (m.isSBO()) {
      m.getSBO().moveTo(*this);
    } else {
      std::swap(mData, m.mData);
    }
    return *this;
  }

  template<typename U, typename... Args >
  typename std::enable_if<
      (sizeof(Impl_type<U>) <= sizeof(Storage)) &&
      std::is_base_of<T, U>::value &&
      std::is_constructible<U, Args...>::value
    >::type
    New(Args&& ... args) {
    destruct();

    // Do a placement new to the local storage and then take the
    // address of the U type and store that in our data pointer.
    Impl<U>* ptr = (Impl<U>*) & getSBO();
    new (ptr) Impl<U>(std::forward<Args>(args)...);
    mData = &(ptr->mU);
  }

  template<typename U, typename... Args >
  typename std::enable_if<
      (sizeof(Impl_type<U>) > sizeof(Storage)) &&
      std::is_base_of<T, U>::value&&
      std::is_constructible<U, Args...>::value
          >::type
      New(Args&& ... args) {
      destruct();

      // this object is too big, use the allocator. Local storage
      // will be unused as denoted by (isSBO() == false).
      mData = new U(std::forward<Args>(args)...);
  }

  bool isSBO() const {
    auto begin = (uint8_t*)this;
    auto end = begin + sizeof(SBO_ptr<T, StorageSize>);
    return
      ((uint8_t*)get() >= begin) &&
      ((uint8_t*)get() < end);
  }

  T* operator->() { return get(); }
  T* get() { return mData; }

  const T* operator->() const { return get(); }
  const T* get() const { return mData; }

  void destruct() {
    if (isSBO())
      // manually call the virtual destructor.
      getSBO().~SBOInterface();
    else if (get())
      // let the compiler call the destructor
      delete get();

    mData = nullptr;
  }

  SBOInterface& getSBO() {
    return *(SBOInterface*)& mStorage;
  }

  const SBOInterface& getSBO() const {
    return *(SBOInterface*)& mStorage;
  }
};

template<typename T, typename U, typename... Args>
typename  std::enable_if<
  std::is_constructible<U, Args...>::value&&
  std::is_base_of<T, U>::value, SBO_ptr<T>>::type
  make_SBO_ptr(Args && ... args) {
  SBO_ptr<T> t;
  t.template New<U>(std::forward<Args>(args)...);
  return (t);
}

enum  SessionMode  {
   Client,
   Server
};


}  // namespace primihub

#endif  // SRC_primihub_UTIL_UTIL_H_
