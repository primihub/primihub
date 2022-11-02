#ifndef __EXPRESS_WRAPPER_H_
#define __EXPRESS_WRAPPER_H_

// #include <glog/logging.h>
#include <pybind11/pybind11.h>
#include <string>

#include "src/primihub/executor/express.h"

namespace py = pybind11;

namespace primihub {

// This class is a wrapper of C++'s express executor with MPC protocol,
// and will be used by python interpreter.
class PyMPCExpressExecutor : public MPCExpressExecutor {
public:
  PyMPCExpressExecutor(uint32_t party_id, string prefix = "No Config",
                       string log_dir = "No Config");

  // These methods are called by python interpreter,
  // ensure call them with below order.

  // 1. Import column owner and dtype.
  void importColumnConfig(py::dict &col_owner, py::dict &col_dtype);

  // 2. Import express.
  void importExpress(const std::string &expr);

  // 3. Import column values.
  void importColumnValues(std::string &col_name, py::list &val_list);

  // 4. Evaluate express with MPC protocol.
  void runMPCEvaluate(const std::string &next_ip, const std::string &prev_ip,
                      uint16_t next_port, uint16_t prev_port);

  // 5. Reveal MPC result to parties.
  py::object revealMPCResult(py::list &party_list);

private:
  inline void importFP64ColumnValues(std::string &col_name, py::list &val_list);
  inline void importI64ColumnValues(std::string &col_name, py::list &val_list);

  uint32_t party_id_;
};

// Warning: this class is only used for test, DON'T USE IT IN ANY APPLICATION.
class PyLocalExpressExecutor : public LocalExpressExecutor {
public:
  PyLocalExpressExecutor(py::object mpc_exec_obj);
  ~PyLocalExpressExecutor();

  void importColumnValues(std::string &name, py::list &val_list);
  void finishImport(void);
  py::object runLocalEvaluate(void);

private:
  inline void importFP64ColumnValues(std::string &owner, py::list &val_list);
  inline void importI64ColumnValues(std::string &owner, py::list &val_list);

  std::map<std::string, std::vector<double>> fp64_val_map_;
  std::map<std::string, std::vector<int64_t>> i64_val_map_;

  PyObject *obj_ptr_;
};

} // namespace primihub

#endif
