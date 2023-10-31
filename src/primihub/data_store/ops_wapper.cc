// ops_wrapper.cpp
// !!! only for test!!!

#include <pybind11/pybind11.h>

#include "ops_wapper.hpp"

PYBIND11_MODULE(pybind11_example, m) {
    
    m.doc() = "primhub ops wrapper"; // Optional module docstring
    m.def("test_unwrap_arrow_pyobject", &unwrap_arrow_pyobject, pybind11::call_guard<pybind11::gil_scoped_release>());
    
    
}
