#include <glog/logging.h>
#include <map>
#include <sstream>
#include <vector>

#include "src/primihub/pybind_warpper/express_wrapper.h"

namespace primihub {
PyMPCExpressExecutor::PyMPCExpressExecutor(uint32_t party_id, string prefix,
                                           string log_dir) {
  if (prefix != "No Config" || log_dir != "No Config") {
    FLAGS_alsologtostderr = true;
    FLAGS_log_prefix = true;
    if (log_dir != "No Config") {
      FLAGS_log_dir = log_dir;
    }
    if (prefix != "No Config") {
      google::SetLogFilenameExtension(prefix.data());
    }
    google::InitGoogleLogging("express_wrapper");
  }
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

void PyMPCExpressExecutor::runMPCEvaluate(const std::string &next_ip,
                                          const std::string &prev_ip,
                                          uint16_t next_port,
                                          uint16_t prev_port) {
  MPCExpressExecutor::initMPCRuntime(party_id_, next_ip, prev_ip, next_port,
                                     prev_port);
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

//
// A brief introduction to usage of MPCExpressExecutor class.
//
// The MPCExpressExecutor class aim to evaluate express like A+B*C-D, and A, B,
// C, D is column name of database in different company or organization, these
// company and organization is called party in MPC. Now the class only support
// three party MPC.
//
// Before introduct the usage of the class, assume something to help understand
// them. First, assume that there are three company which act as three party in
// MPC. Second, column A belong to the first party (called party 0), column B
// belong to second party (called party 1), column C, D belong to third party
// (called party 2). Third, all value of column A is float value, all value of
// column B is float value, all value of column C is float value.
//
// The usage of this class is that:
// 0. Create class instance.
//    for every party:
//      # party_id is every party's party id, must not be the same.
//      mpc_exec = pybind_mpc.MPCExpressExecutor(party_id)
//
// 1.Import column's owner and data type.
//    for every party:
//      # define a dict that save all column's owner (party id):
//      col_owner = {"A": 0, "B": 1, "C": 2, "D": 2}
//
//      # define a dict that save all column's dtype, notice that 'true' means
//      # this column has data type float:
//      col_dtype = {"A": true, "B": true, "C": true, "D": true}
//
//      # import them into class instance:
//      mpc_exec.import_column_config(col_owner, col_dtype)
//
//      # Notice all party should provide the same col_owner and col_dtype to
//      # this two interface.
//
// 2.Import express.
//    for every party:
//      expr = "A+B*C-D"
//      mpc_exec.import_express(expr)
//      # expr imported in every party should be the same.
//
// 3.Import column's value.
//    for party 0:
//      val_a = []
//      # do something to fill this list.
//      mpc_exec.import_column_values("A", val_a)
//
//    for party 1:
//      val_b = []
//      # do something to fill this list.
//      mpc_exec.import_column_values("B", val_b)
//
//    for party 2:
//      val_c = []
//      val_d = []
//      # do something to fill the two list.
//      mpc_exec.import_column_values("C", val_c)
//      mpc_exec.import_column_values("D", val_d)
//
// 4.Evaluate with MPC protocol.
//    for every party:
//      mpc_exec.evaluate()
//
// 5.Get evaluate result.
//    for every party:
//      # define a list to hold which party will get evaluate result, and
//      # reveal_list in every party should be the same.
//      reveal_list = [0, 1, 2] # a list of party id.
//
//      # reveal evaluate result:
//      mpc_exec.reveal_mpc_result(reveal_list)
//
PYBIND11_MODULE(pybind_mpc, m) {
  // clang-format off
  py::class_<primihub::PyMPCExpressExecutor>(m, "MPCExpressExecutor")
      .def(py::init<uint32_t,string,string>())
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
