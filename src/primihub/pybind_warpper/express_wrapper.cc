#include <map>
#include <sstream>
#include <vector>

#include <glog/logging.h>

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

void PyMPCExpressExecutor::importColumnConfig(py::dict &col_owner,
                                              py::dict &col_dtype) {
  initColumnConfig(party_id_);

  std::string col_name;
  uint8_t party_id;

  for (std::pair<py::handle, py::handle> &item : col_owner) {
    col_name = item.first.cast<std::string>();
    party_id = item.second.cast<uint8_t>();
    if (importColumnOwner(col_name, party_id)) {
      std::stringstream ss;
      ss << "Import column " << col_name << " and it's owner's party id "
         << party_id << " failed.";
      throw std::runtime_error(ss.str());
    }
  }

  for (std::pair<py::handle, py::handle> &item : col_dtype) {
     
  }
}

}; // namespace primihub
