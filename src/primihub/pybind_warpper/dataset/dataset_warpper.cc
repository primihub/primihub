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

#include "dataset_warpper.hpp"

PYBIND11_MODULE(primihub_dataset_warpper, m) {

    m.doc() = "primhub dataset API wrapper"; // Optional module docstring
    m.def("test_unwrap_arrow_pyobject", &test_unwrap_arrow_pyobject, pybind11::call_guard<pybind11::gil_scoped_release>());
    m.def("reg_arrow_table_as_ph_dataset", &reg_arrow_table_as_ph_dataset, pybind11::call_guard<pybind11::gil_scoped_release>());
}
