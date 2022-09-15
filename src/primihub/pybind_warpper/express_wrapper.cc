#include <glog/logging.h>
#include <map>
#include <sstream>
#include <vector>

#include "src/primihub/pybind_warpper/express_wrapper.h"

namespace primihub {
PyMPCExpressExecutor::PyMPCExpressExecutor(uint8_t party_id) {
  if ((party_id == 0) || (party_id == 1) || (party_id == 2)) {
    this->party_id_ = party_id;
  } else {
    std::stringstream ss;
    ss << "Party id should be 0 or 1 or 2, current party id is " << party_id
       << ".";
    throw std::invalid_argument(ss.str());
  }
}

void PyMPCExpressExecutor::importExpress(const std::string &expr) {
  if (MPCExpressExecutor::importExpress(expr)) {
    std::stringstream ss;
    ss << "Import express " << expr << " failed.";
    throw std::runtime_error(ss.str());
  }

  MPCExpressExecutor::resolveRunMode();
}

void PyMPCExpressExecutor::importColumnConfig(py::dict &col_owner,
                                              py::dict &col_dtype) {
  initColumnConfig(party_id_);

  std::string col_name;
  uint8_t party_id;

  for (const std::pair<py::handle, py::handle> &item : col_owner) {
    col_name = item.first.cast<std::string>();
    party_id = item.second.cast<uint8_t>();
    if (importColumnOwner(col_name, party_id)) {
      std::stringstream ss;
      ss << "Import column " << col_name << " and it's owner's party id "
         << party_id << " failed.";
      throw std::runtime_error(ss.str());
    }
  }

  bool is_fp64 = false;
  for (const std::pair<py::handle, py::handle> &item : col_dtype) {
    is_fp64 = item.second.cast<bool>();
    col_name = item.first.cast<std::string>();

    if (importColumnDtype(col_name, is_fp64)) {
      std::string dtype_str;
      if (is_fp64 == false)
        dtype_str = "I64";
      else
        dtype_str = "FP64";

      std::stringstream ss;
      ss << "Import column " << col_name << " and it's dtype " << dtype_str
         << " failed.";
      throw std::runtime_error(ss.str());
    }
  }

  return;
}

void PyMPCExpressExecutor::importI64ColumnValues(std::string &name,
                                                 py::list &val_list) {
  std::vector<int64_t> val_vec;
  for (auto v : val_list)
    val_vec.emplace_back(v.cast<int64_t>());

  if (importColumnValues(name, val_vec)) {
    std::stringstream ss;
    ss << "Import column " << name << "'s value failed, dtype is I64.";
    throw std::runtime_error(ss.str());
  }

  return;
}

void PyMPCExpressExecutor::importFP64ColumnValues(std::string &name,
                                                  py::list &val_list) {
  std::vector<double> val_vec;
  for (auto v : val_list)
    val_vec.emplace_back(v.cast<double>());

  if (importColumnValues(name, val_vec)) {
    std::stringstream ss;
    ss << "Import column " << name << "'s value failed, dtype is FP64.";
    throw std::runtime_error(ss.str());
  }

  return;
}

void PyMPCExpressExecutor::runMPCEvaluate(uint8_t party_id,
                                          const std::string &ip,
                                          uint16_t next_port,
                                          uint16_t prev_port) {
  MPCExpressExecutor::initMPCRuntime(party_id, ip, next_port, prev_port);
  if (MPCExpressExecutor::runMPCEvaluate())
    throw std::runtime_error("evaluate express with mpc failed.");
}

py::object PyMPCExpressExecutor::revealMPCResult(py::list &party_list) {
  std::vector<uint8_t> party;
  for (auto v : party_list)
    party.emplace_back(v.cast<uint8_t>());

  if (isFP64RunMode()) {
    std::vector<double> vec;
    MPCExpressExecutor::revealMPCResult(party, vec);

    if (vec.size() == 0) {
      return py::cast<py::none>(Py_None);
    } else {
      py::list result;
      for (auto v : vec)
        result.append(v);

      return result;
    }
  } else {
    std::vector<int64_t> vec;
    MPCExpressExecutor::revealMPCResult(party, vec);

    if (vec.size() == 0) {
      return py::cast<py::none>(Py_None);
    } else {
      py::list result;
      for (auto v : vec)
        result.append(v);

      return result;
    }
  }
}

}; // namespace primihub

PYBIND11_MODULE(pympc, m) {
  py::class_<primihub::PyMPCExpressExecutor>(m, "Executor")
      .def(py::init<uint8_t>())
      .def("import_column_config",
           &primihub::PyMPCExpressExecutor::importColumnConfig)
      .def("import_column_fp64_values",
           &primihub::PyMPCExpressExecutor::importFP64ColumnValues)
      .def("import_column_i64_values",
           &primihub::PyMPCExpressExecutor::importI64ColumnValues)
      .def("reveal_mpc_result",
           &primihub::PyMPCExpressExecutor::revealMPCResult)
      .def("import_express", 
           &primihub::PyMPCExpressExecutor::importExpress)
      .def("evaluate_with_mpc", 
           &primihub::PyMPCExpressExecutor::runMPCEvaluate);
}
