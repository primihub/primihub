#include <glog/logging.h>
#include <map>
#include <sstream>
#include <vector>

#include "src/primihub/pybind_warpper/express_wrapper.h"

namespace primihub {
PyMPCExpressExecutor::PyMPCExpressExecutor(uint32_t party_id) {
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
    party_id = item.second.cast<uint32_t>();
    if (importColumnOwner(col_name, party_id)) {
      std::stringstream ss;
      ss << "Import column " << col_name << " and it's owner's party id "
         << party_id << " failed.";
      throw std::runtime_error(ss.str());
    }
  }

  bool is_fp64 = 0;
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

  if (MPCExpressExecutor::importColumnValues(name, val_vec)) {
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

  if (MPCExpressExecutor::importColumnValues(name, val_vec)) {
    std::stringstream ss;
    ss << "Import column " << name << "'s value failed, dtype is FP64.";
    throw std::runtime_error(ss.str());
  }

  return;
}

void PyMPCExpressExecutor::importColumnValues(std::string &name,
                                              py::list &val_list) {
  MPCExpressExecutor::InitFeedDict();

  PyObject *obj = val_list[0].ptr();
  if (PyFloat_Check(obj))
    importFP64ColumnValues(name, val_list);
  else
    importI64ColumnValues(name, val_list);
}

void PyMPCExpressExecutor::runMPCEvaluate(const std::string &ip,
                                          uint16_t next_port,
                                          uint16_t prev_port) {
  MPCExpressExecutor::initMPCRuntime(party_id_, ip, next_port, prev_port);
  if (MPCExpressExecutor::runMPCEvaluate())
    throw std::runtime_error("evaluate express with mpc failed.");
}

py::object PyMPCExpressExecutor::revealMPCResult(py::list &party_list) {
  std::vector<uint32_t> party;
  for (auto v : party_list)
    party.emplace_back(v.cast<uint32_t>());

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

PyLocalExpressExecutor::PyLocalExpressExecutor(py::object mpc_exec_obj) {
  PyObject *obj = mpc_exec_obj.ptr();
  Py_INCREF(obj);

  PyMPCExpressExecutor *mpc_exec = mpc_exec_obj.cast<PyMPCExpressExecutor *>();
  LocalExpressExecutor::setMPCExpressExecutor(mpc_exec);

  obj_ptr_ = obj;
}

PyLocalExpressExecutor::~PyLocalExpressExecutor() { Py_DECREF(obj_ptr_); }

void PyLocalExpressExecutor::importFP64ColumnValues(std::string &owner,
                                                    py::list &val_list) {
  std::vector<double> val_vec;
  for (auto v : val_list)
    val_vec.push_back(v.cast<double>());

  fp64_val_map_.insert(std::make_pair(owner, val_vec));
}

void PyLocalExpressExecutor::importI64ColumnValues(std::string &owner,
                                                   py::list &val_list) {
  std::vector<int64_t> val_vec;
  for (auto v : val_list)
    val_vec.push_back(v.cast<int64_t>());

  i64_val_map_.insert(std::make_pair(owner, val_vec));
}

void PyLocalExpressExecutor::importColumnValues(std::string &owner,
                                                py::list &val_list) {
  PyObject *obj = val_list[0].ptr();
  if (PyFloat_Check(obj))
    importFP64ColumnValues(owner, val_list);
  else
    importI64ColumnValues(owner, val_list);
}

void PyLocalExpressExecutor::finishImport(void) {
  if (LocalExpressExecutor::isFP64RunMode())
    LocalExpressExecutor::init(fp64_val_map_);
  else
    LocalExpressExecutor::init(i64_val_map_);
}

py::object PyLocalExpressExecutor::runLocalEvaluate(void) {
  if (LocalExpressExecutor::runLocalEvaluate()) {
    throw std::runtime_error("run express failed.");
    return py::cast<py::none>(Py_None);
  }

  if (LocalExpressExecutor::isFP64RunMode()) {
    std::vector<double> result;
    LocalExpressExecutor::getFinalVal(result);

    py::list val_list;
    for (auto v : result)
      val_list.append(v);
    return val_list;
  } else {
    std::vector<int64_t> result;
    LocalExpressExecutor::getFinalVal(result);

    py::list val_list;
    for (auto v : result)
      val_list.append(v);
    return val_list;
  }
}

}; // namespace primihub

PYBIND11_MODULE(pympc, m) {
  // clang-format off
  py::class_<primihub::PyMPCExpressExecutor>(m, "MPCExpressExecutor")
      .def(py::init<uint32_t>())
      .def("import_column_config",
           &primihub::PyMPCExpressExecutor::importColumnConfig)
      .def("import_column_values",
           &primihub::PyMPCExpressExecutor::importColumnValues)
      .def("reveal_mpc_result",
           &primihub::PyMPCExpressExecutor::revealMPCResult)
      .def("import_express", 
           &primihub::PyMPCExpressExecutor::importExpress)
      .def("evaluate",
           &primihub::PyMPCExpressExecutor::runMPCEvaluate);

  py::class_<primihub::PyLocalExpressExecutor>(m, "LocalExpressExecutor")
      .def(py::init<py::object>())
      .def("import_column_values",
           &primihub::PyLocalExpressExecutor::importColumnValues)
      .def("finish_import", 
           &primihub::PyLocalExpressExecutor::finishImport)
      .def("evaluate", 
           &primihub::PyLocalExpressExecutor::runLocalEvaluate);
  // clang-format on
}
