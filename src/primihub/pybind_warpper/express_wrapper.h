#ifdef __EXPRESS_WRAPPER_H_
#define __EXPRESS_WRAPPER_H_

#include <pybind11/pybind11.h>
#include <string>

#include "src/primihub/executor/express.h"

namespace primihub {
class PyMPCExpressExecutor : public MPCExpressExecutor {
public:
  PyMPCExpressExecutor(uint8_t party_id);
  void importColumnConfig(py::dict &col_owner, py::dict &col_dtype);
  void importColumnValues(std::string col_name, py::list &val_list);
  void revealMPCResult(py::list &party_list);

private:
  uint8_t party_id_;
}
} // namespace primihub

#endif
