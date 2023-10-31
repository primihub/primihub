/*
 Copyright 2022 PrimiHub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */



#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include <boost/beast.hpp>
#include <boost/di.hpp>
#include <boost/thread/thread.hpp>

#include <libp2p/common/hexutil.hpp>
#include <libp2p/injector/kademlia_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/protocol/identify/identify.hpp>
#include <libp2p/protocol/identify/identify_msg_processor.hpp>
#include <libp2p/outcome/outcome.hpp>

#include "src/primihub/p2p/node_stub.h"


using namespace libp2p::protocol::kademlia;
using namespace boost::di;




namespace primihub::p2p
{

namespace {
const std::string logger_config(R"(
# ----------------
sinks:
  - name: console
    type: console
    color: true
groups:
  - name: main
    sink: console
    level: critical
    children:
      - name: libp2p
# ----------------
  )");

   class PermissiveKeyValidator
      : public libp2p::crypto::validator::KeyValidator {
   public:
    libp2p::outcome::result<void> validate(const libp2p::crypto::PrivateKey &key) const override {
      return libp2p::outcome::success();
    }
    libp2p::outcome::result<void> validate(const libp2p::crypto::PublicKey &key) const override {
      return libp2p::outcome::success();
    }
    libp2p::outcome::result<void> validate(const libp2p::crypto::KeyPair &keys) const override {
      return libp2p::outcome::success();
    }
  };
} // namespace

    NodeStub::NodeStub(const std::vector<std::string>& bootstrap_nodes_addr)
    {
        this->bootstrap_nodes_addr_ = bootstrap_nodes_addr;
        //Init DatasetService with nodelet as stub
        onInit();
    }

    NodeStub::~NodeStub()
    {
        // TODO stop node and release all protocol/service resources
    }

    void NodeStub::onInit()
    {
        // prepare log system
        auto logging_system = std::make_shared<soralog::LoggingSystem>(
            std::make_shared<soralog::ConfiguratorFromYAML>(
                // Original LibP2P logging config
                std::make_shared<libp2p::log::Configurator>(),
                // Additional logging config for application
                logger_config));
        auto r = logging_system->configure();

        if (not r.message.empty())
        {
            (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
        }
        if (r.has_error)
        {
            exit(EXIT_FAILURE);
        }

        libp2p::log::setLoggingSystem(logging_system);
        libp2p::log::setLevelOfGroup("NodeStub", soralog::Level::CRITICAL);

        // resulting PeerId should be
        // 12D3KooWEgUjBV5FJAuBSoNMRYFRHjV7PjZwRQ7b43EKX9g7D6xV
        node_kp_ = {
            // clang-format off
            .publicKey = {{
                .type = libp2p::crypto::Key::Type::Ed25519,
                .data = libp2p::common::unhex("48453469c62f4885373099421a7365520b5ffb0d93726c124166be4b81d852e6").value()
            }},
            .privateKey = {{
                .type = libp2p::crypto::Key::Type::Ed25519,
                .data = libp2p::common::unhex("4a9361c525840f7086b893d584ebbe475b4ec7069951d2e897e8bceb0a3f35ce").value()
            }},
            // clang-format on
        };

        kademlia_config_.randomWalk.enabled = true;
        kademlia_config_.randomWalk.interval = std::chrono::seconds(300);
        kademlia_config_.requestConcurency = 20;

        // get bootstrap nodes
        bootstrap_nodes_ = [&]
        {
            std::unordered_map<libp2p::peer::PeerId,
                               std::vector<libp2p::multi::Multiaddress>>
                addresses_by_peer_id;

            for (auto &address : this->bootstrap_nodes_addr_)
            {
                auto ma = libp2p::multi::Multiaddress::create(address).value();
                auto peer_id_base58 = ma.getPeerId().value();
                auto peer_id = libp2p::peer::PeerId::fromBase58(peer_id_base58).value();

                addresses_by_peer_id[std::move(peer_id)].emplace_back(std::move(ma));
            }

            std::vector<libp2p::peer::PeerInfo> v;
            v.reserve(addresses_by_peer_id.size());
            for (auto &i : addresses_by_peer_id)
            {
                v.emplace_back(libp2p::peer::PeerInfo{
                    .id = i.first, .addresses = {std::move(i.second)}});
            }
            return v;
        }(); // get boostarp nodes lambda end
    }

    void NodeStub::start(std::string_view ma_str)
    {
        try
        {
            auto injector = libp2p::injector::makeHostInjector(
                // libp2p::injector::useKeyPair(kp), // TODO Use predefined keypair
                libp2p::injector::makeKademliaInjector(
                    libp2p::injector::useKademliaConfig(kademlia_config_)));

            auto ma = libp2p::multi::Multiaddress::create(ma_str).value(); // NOLINT

            this->io_context_ = injector.create<std::shared_ptr<boost::asio::io_context>>();

            host_ = injector.create<std::shared_ptr<libp2p::Host>>();

            auto self_id = host_->getId();

            std::cerr << self_id.toBase58() << " * started" << std::endl;

            // Create kademlia
            this->kademlia_ =
                injector
                    .create<std::shared_ptr<libp2p::protocol::kademlia::Kademlia>>();

            // ======= Identify =========
            auto key_marshaller =
                std::make_shared<libp2p::crypto::marshaller::KeyMarshallerImpl>(std::make_shared<PermissiveKeyValidator>());

            std::shared_ptr<libp2p::protocol::IdentifyMessageProcessor> id_msg_processor =
                std::make_shared<libp2p::protocol::IdentifyMessageProcessor>(
                    *host_, host_->getNetwork().getConnectionManager(),
                    *std::dynamic_pointer_cast<libp2p::host::BasicHost>(host_)->idmgr_.get(), key_marshaller);

            identify_ =
                std::make_shared<libp2p::protocol::Identify>(*host_, id_msg_processor, host_->getBus());

            // ================
            // Start the host
            // Post async task to run the host
            io_context_->post([&, ma]
            {
                auto listen = host_->listen(ma);

                if (not listen)
                {
                    std::cerr << "Cannot listen address " << ma.getStringAddress().data()
                              << ". Error: " << listen.error().message() << std::endl;
                    std::exit(EXIT_FAILURE);
                }

                for (auto &bootstrap_node : bootstrap_nodes_)
                {
                    kademlia_->addPeer(bootstrap_node, true);
                }
                // start p2p host
                host_->start();

                // start identify protocol
                identify_->start();

                // start kademlia protocol
                kademlia_->start();
            });
            threadpool_.create_thread(
                boost::bind(&boost::asio::io_context::run, io_context_));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    // start put value to kademlia
    void NodeStub::putDHTValue(libp2p::protocol::kademlia::ContentId key, std::string value) {

        io_context_->post([&,key,value] {
            auto sid = libp2p::multi::ContentIdentifierCodec::toString(libp2p::multi::ContentIdentifierCodec::decode(key.data).value());
            std::cout << "Put value, key string is : " << sid.value() << std::endl;
            std::istringstream is_value( value );
            libp2p::protocol::kademlia::Value value_obj;
            uint8_t number;
            while ( is_value >> number )  // FIXME lost text space in string
                value_obj.push_back( number );

            auto res = kademlia_->putValue(key, value_obj); // TODO need get success result
            if (res) {
                std::cout << "Put value success, value length:" << value_obj.size() <<std::endl;
            } else {
                std::cout << "Put value failed" << std::endl;
            }

        });
    }

    void NodeStub::putDHTValue(std::string key, std::string value) {
        // start put value to kademlia
        io_context_->post([&,key,value] {
            libp2p::protocol::kademlia::ContentId content_id(key);
            auto sid = libp2p::multi::ContentIdentifierCodec::toString(libp2p::multi::ContentIdentifierCodec::decode(content_id.data).value());
            std::cout << "Put value, key string is : " << sid.value() << std::endl;
            std::istringstream is_value( value );
            libp2p::protocol::kademlia::Value value_obj;
            uint8_t number;
            while ( is_value >> number )  // FIXME lost space in string
                value_obj.push_back( number );

            auto res = kademlia_->putValue(content_id, value_obj); // TODO need get success result
            if (res) {
                std::cout << "Put value success, value length:" << value_obj.size() <<std::endl;
            } else {
                std::cout << "Put value failed" << std::endl;
            }
        });
    }

    void NodeStub::getDHTValue(std::string key, libp2p::protocol::kademlia::FoundValueHandler handler) {
        // start get value from kademlia
        this->io_context_->post([&, key, handler] {
             libp2p::protocol::kademlia::ContentId content_id(key);
            auto res = kademlia_->getValue(content_id, [&](libp2p::outcome::result<libp2p::protocol::kademlia::Value> result) {
                if (result.has_error()) {
                    std::cerr << " ! getVHandler failed: " << result.error().message()
                            << std::endl;
                    return;
                } else {
                    auto r = result.value();
                    std::stringstream rs;
                    std::copy(r.begin(), r.end(), std::ostream_iterator<uint8_t>(rs, ""));
                    std::cout << "🚩🚩 "<< rs.str() <<std::endl;
                }
            });
             if (not res) {
                std::cerr << "Cannot get value: " << res.error().message() << std::endl;
                return;
            } else {
                std::cout << "Get value success" << std::endl;
            }
        });
    }



     void NodeStub::getDHTValue(libp2p::protocol::kademlia::ContentId key,
                                libp2p::protocol::kademlia::FoundValueHandler handler) {
        // start get value from kademlia
        this->io_context_->post([&, key, handler] {
            auto res = kademlia_->getValue(key, handler);
             if (not res) {
                std::cerr << "Cannot get value: " << res.error().message() << std::endl;
                return;
            } else {
                std::cout << "Get value request success " << std::endl;
            }
        });
    }


} // namespace primihub::p2p
