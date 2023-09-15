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

#include <pybind11/pybind11.h>
#include <string>
#include <string_view>
#include "src/primihub/util/network/link_factory.h"
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/network/grpc_link_context.h"
#include "src/primihub/common/common.h"
#include "src/primihub/common/config/config.h"

namespace py = pybind11;

using primihub::Node;
using primihub::common::CertificateConfig;
using primihub::network::GrpcChannel;
using primihub::network::GrpcLinkContext;
using primihub::network::IChannel;
using primihub::network::LinkContext;
using primihub::network::LinkFactory;
using primihub::network::LinkMode;

PYBIND11_MODULE(linkcontext, m)
{
    py::enum_<LinkMode>(m, "LinkMode", py::arithmetic())
        .value("GRPC", LinkMode::GRPC, "connection with grpc")
        .value("RAW_SOCKET", LinkMode::RAW_SOCKET, "connect with socket");

    py::class_<CertificateConfig>(m, "CertificateConfig")
        .def(py::init<const std::string&, const std::string&, const std::string&>());

    py::class_<Node>(m, "Node")
        .def(py::init<const std::string &, const uint32_t, bool, const std::string &>())
        .def("to_string", &Node::to_string, py::call_guard<py::gil_scoped_release>());

    py::class_<LinkFactory>(m, "LinkFactory")
        .def_static("createLinkContext", &LinkFactory::createLinkContext);

    py::class_<LinkContext>(m, "LinkContext")
        .def("setTaskInfo", &LinkContext::setTaskInfo)
        .def("getChannel", &LinkContext::getChannel)
        .def("setRecvTimeout", &LinkContext::setRecvTimeout)
        .def("initCertificate", py::overload_cast<const std::string&,
                const std::string&, const std::string&>(&LinkContext::initCertificate))
        .def("setSendTimeout", &LinkContext::setSendTimeout);

    py::class_<GrpcLinkContext, LinkContext>(m, "GrpcLinkContext")
        .def(py::init());

    py::class_<IChannel, std::shared_ptr<IChannel>>(m, "IChannel")
        .def("send",
            py::overload_cast<const std::string&, std::string_view>(&IChannel::send_wrapper),
            py::call_guard<py::gil_scoped_release>())
        .def("send",
            py::overload_cast<const std::string&, const std::string&>(&IChannel::send_wrapper),
            py::call_guard<py::gil_scoped_release>())
        .def("recv", [](IChannel& self, const std::string& key) {
          std::string recv_str;
          {
            py::gil_scoped_release release;
            recv_str = self.forwardRecv(key);
          }
          if (recv_str.empty()) {
            throw pybind11::value_error("received data encountes error");
          }
          return py::bytes(recv_str);
        });

    py::class_<GrpcChannel, IChannel, std::shared_ptr<GrpcChannel>>(m, "GrpcChannel")
        .def(py::init<const Node &, LinkContext *>());
}
