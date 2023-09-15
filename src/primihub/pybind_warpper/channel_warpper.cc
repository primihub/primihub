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

#include "src/primihub/util/network/socket/ioservice.h"
#include "src/primihub/util/network/socket/session.h"
#include "src/primihub/util/network/socket/channel.h"
#include "src/primihub/util/util.h"


using primihub::SessionMode;
using primihub::IOService;
using primihub::Session;
using primihub::Channel;

namespace py = pybind11;


PYBIND11_MODULE(primihub_channel, m) {
  py::class_<IOService>(m, "IOService")
        .def(py::init<uint64_t>());

  py::enum_<SessionMode>(m, "SessionMode")
        .value("Client", SessionMode::Client)
        .value("Server", SessionMode::Server)
        .export_values();

  py::class_<Session>(m, "Session")
        .def(py::init<IOService &, std::string, SessionMode, std::string>())
        .def("addChannel", &Session::addChannel);

  py::class_<Channel>(m, "Channel")
        .def(py::init<>())
        .def("send", &Channel::send<std::string>)
        .def("asyncSendCopy", &Channel::asyncSendCopy<std::string>)
        .def("recv", [](Channel &self) {
              std::string recv_str;
              self.recv(recv_str);
              return recv_str;
        })
        .def("close", &Channel::close);

}
