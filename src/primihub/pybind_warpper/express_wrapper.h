#ifndef __EXPRESS_WRAPPER_H_
#define __EXPRESS_WRAPPER_H_

#include <pybind11/pybind11.h>
#include <string>

#include "src/primihub/executor/express.h"

namespace py = pybind11;

namespace primihub {
// This class is a wrapper of C++'s express executor with MPC protocol, 
// and will be used by python interpreter.
class PyMPCExpressExecutor : public MPCExpressExecutor {
public:
  PyMPCExpressExecutor(uint32_t party_id);
  
  // These methods are called by python interpreter, 
  // ensure call them with below order.
 
  // 1. Import column owner and dtype.
  void importColumnConfig(py::dict &col_owner, py::dict &col_dtype);
  
  // 2. Import express.
  void importExpress(const std::string &expr);
  
  // 3. Import column values.
  void importFP64ColumnValues(std::string &col_name, py::list &val_list);
  void importI64ColumnValues(std::string &col_name, py::list &val_list);
  
  // 4. Evaluate express with MPC protocol.
  void runMPCEvaluate(uint32_t party_id, const std::string &ip,
                      uint16_t next_port, uint16_t prev_port);
  
  // 5. Reveal MPC result to parties. 
  py::object revealMPCResult(py::list &party_list);

private:
  uint32_t party_id_;
};
} // namespace primihub

#endif
