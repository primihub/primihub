
// Copyright (C) 2022 <primihub.com>

#ifndef SRC_PRIMIHUB_UTIL_NETWORK_SOCKET_COMMPKG_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_SOCKET_COMMPKG_H_

#include <unordered_map>
#include <string>
#include <iostream>

#include "src/primihub/util/network/socket/channel.h"

namespace primihub {
    class Channel;

    struct CommPkgBase {  
        public:
           CommPkgBase();
           virtual ~CommPkgBase() {};
           std::unordered_map<std::string, Channel> mChannels;

    };

    struct ABY3CommPkg : public CommPkgBase  // FIXME move to ABY3 package
    {
    public:
        
        inline Channel &mPrev()  { return mChannels.at("prev"); }
        inline Channel &mNext()  { return mChannels.at("next"); }

        void setPrev(Channel channel) { mChannels["prev"] = channel; }
        void setNext(Channel channel) { mChannels["next"] = channel; }

        // RO_PROPERTY(ABY3CommPkg, Channel, GetPrev) mPrev;
        // RO_PROPERTY(ABY3CommPkg, Channel, GetNext) mNext;
        
        ABY3CommPkg() /*: mPrev(this), mNext(this)*/
        {
            mChannels.emplace("prev", Channel());
            mChannels.emplace("next", Channel());
        }
        ABY3CommPkg(Channel prev, Channel next)
        {
            mChannels.emplace("prev", prev);
            mChannels.emplace("next", next);
        }
        ABY3CommPkg(ABY3CommPkg const &other)
        {
            mChannels.emplace("prev", other.mChannels.at("prev"));
            mChannels.emplace("next", other.mChannels.at("next"));
        }
        
        ABY3CommPkg& operator=(const ABY3CommPkg &other)
        {
            mChannels.emplace("prev", other.mChannels.at("prev"));
            mChannels.emplace("next", other.mChannels.at("next"));
            return *this;
        }
    };

    using CommPkg = ABY3CommPkg;  // FIXME move to ABY3 package

} // namespace primihub

#endif // SRC_PRIMIHUB_UTIL_NETWORK_SOCKET_COMMPKG_H_
