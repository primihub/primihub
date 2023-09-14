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
// #include "src/primihub/task/pybind_wrapper/psi_wrapper.h"
#include "src/primihub/task/pybind_wrapper/mpc_task_wrapper.h"
namespace py = pybind11;

using MPCExecutor = primihub::task::MPCExecutor;


PYBIND11_MODULE(ph_secure_lib, m) {
  py::class_<MPCExecutor>(m, "MPCExecutor")
    .def(py::init<const std::string&, const std::string&>())
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
        return result;});
}
