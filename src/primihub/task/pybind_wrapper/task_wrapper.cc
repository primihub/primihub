/*
* Copyright (c) 2023 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "src/primihub/common/common.h"
#include "src/primihub/common/config/config.h"
#include "src/primihub/task/pybind_wrapper/psi_wrapper.h"
#include "src/primihub/task/pybind_wrapper/mpc_task_wrapper.h"
namespace py = pybind11;

using MPCExecutor = primihub::task::MPCExecutor;
using PSIExecutor = primihub::task::PsiExecutor;

PYBIND11_MODULE(ph_secure_lib, m) {
  py::class_<MPCExecutor>(m, "MPCExecutor")
    .def(py::init<const std::string&, const std::string&,
                  const std::string&, const std::string&, const std::string&>())
    .def(py::init<const std::string&>())
    .def("max", [](MPCExecutor& self, const std::vector<double>& input) {
        std::vector<double> result;
        primihub::retcode ret;
        {
          py::gil_scoped_release release;
          ret = self.Max(input, &result);
        }
        if (ret != primihub::retcode::SUCCESS) {
          throw pybind11::value_error("receive data encountes error");
        }
        return result;})
    .def("min", [](MPCExecutor& self, const std::vector<double>& input) {
        std::vector<double> result;
        primihub::retcode ret;
        {
          py::gil_scoped_release release;
          ret = self.Min(input, &result);
        }
        if (ret != primihub::retcode::SUCCESS) {
          throw pybind11::value_error("receive data encountes error");
        }
        return result;})
    .def("avg", [](MPCExecutor& self,
                   const std::vector<double>& input,
                   const std::vector<int64_t>& col_rows) {
        std::vector<double> result;
        primihub::retcode ret;
        {
          py::gil_scoped_release release;
          ret = self.Avg(input, col_rows, &result);
        }
        if (ret != primihub::retcode::SUCCESS) {
          throw pybind11::value_error("receive data encountes error");
        }
        return result;})
    .def("sum", [](MPCExecutor& self, const std::vector<double>& input) {
        std::vector<double> result;
        primihub::retcode ret;
        {
          py::gil_scoped_release release;
          ret = self.Sum(input, &result);
        }
        if (ret != primihub::retcode::SUCCESS) {
          throw pybind11::value_error("receive data encountes error");
        }
        return result;})
    .def("stop_task",
         &MPCExecutor::StopTask, py::call_guard<py::gil_scoped_release>());

  py::class_<PSIExecutor>(m, "PSIExecutor")
    .def(py::init<const std::string&,
                  const std::string&, const std::string&, const std::string&>())
    .def("run_as_string", [](PSIExecutor& self,
                   const std::vector<std::string>& input,
                   const std::vector<std::string>& parties,
                   const std::string& receiver,
                   bool broadcast,
                   const std::string& protocol) {
          std::vector<std::string> result;
          primihub::retcode ret;
          {
            py::gil_scoped_release release;
            result = self.RunPsi(input, parties, receiver, broadcast, protocol);
          }
          return result;
        })
    .def("run_as_integer", [](PSIExecutor& self,
                   const std::vector<int64_t>& input,
                   const std::vector<std::string>& parties,
                   const std::string& receiver,
                   bool broadcast,
                   const std::string& protocol) {
          std::vector<int64_t> result;
          do {
            py::gil_scoped_release release;
            std::vector<std::string> input_str;
            input_str.reserve(input.size());
            std::unordered_map<std::string, int64_t> map_info(input.size());
            for (size_t i = 0; i < input.size(); i++) {
              std::string item = std::to_string(input[i]);
              if (map_info.find(item) != map_info.end()) {
                continue;
              }
              input_str.push_back(item);
              map_info[item] = input[i];
            }
            primihub::retcode ret;
            auto tmp_result =
                self.RunPsi(input_str, parties, receiver, broadcast, protocol);
            if (tmp_result.empty()) {
              break;
            }
            result.reserve(tmp_result.size());
            for (const auto& item : tmp_result) {
              auto it = map_info.find(item);
              if (it == map_info.end()) {
                continue;
              }
              result.push_back(it->second);
            }
          } while (0);
          return result;
        });
}
