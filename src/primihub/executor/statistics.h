#ifndef _STATISTICS_EXECUTOR_H_
#define _STATISTICS_EXECUTOR_H_

#include "src/primihub/common/common.h"
#include "src/primihub/common/type/type.h"
#include "src/primihub/data_store/dataset.h"
#include "src/primihub/operator/aby3_operator.h"

#include <arrow/api.h>
#include <arrow/array.h>
#include <arrow/result.h>

namespace primihub {
#ifndef MPC_SOCKET_CHANNEL
class MPCStatisticsOperator {
public:
  MPCStatisticsOperator() {}
  virtual ~MPCStatisticsOperator() {}

  enum MPCStatisticsType {
    AVG,
    SUM,
    MAX,
    MIN,
    UNKNOWN,
  };

  virtual retcode getResult(eMatrix<double> &col_avg) {
    return _always_error("Method 'getResult' not implement.");
  }

  virtual retcode run(std::shared_ptr<primihub::Dataset> &dataset,
                      const std::vector<std::string> &columns,
                      const std::map<std::string, ColumnDtype> &col_dtype) {
    return _always_error("Method 'run' not implement.");
  }

  virtual retcode setupChannel(uint16_t party_id, MpcChannel &next,
                               MpcChannel &prev) {
    return _always_error("Method 'setupChannel' not implement.");
  }

  static std::string statisticsTypeToString(const MPCStatisticsType &type) {
    std::string str;

    switch (type) {
    case MPCStatisticsType::AVG:
      str = "AVG";
      break;
    case MPCStatisticsType::SUM:
      str = "SUM";
      break;
    case MPCStatisticsType::MAX:
      str = "MAX";
      break;
    case MPCStatisticsType::MIN:
      str = "MIN";
      break;
    default:
      str = "UNKNOWN";
      break;
    }

    return str;
  }

private:
  retcode _always_error(const std::string msg) {
    LOG(ERROR) << msg;
    return retcode::FAIL;
  }
};

class MPCSumOrAvg : public MPCStatisticsOperator {
public:
  MPCSumOrAvg(const MPCStatisticsType &type) {
    use_mpc_div_ = false;
    if (type == MPCStatisticsType::AVG) {
      avg_result_ = true;
    } else {
      avg_result_ = false;
    }
  };

  virtual ~MPCSumOrAvg() {
    mpc_op_.reset();
  }

  retcode getResult(eMatrix<double> &col_avg) override;

  retcode run(std::shared_ptr<primihub::Dataset> &dataset,
              const std::vector<std::string> &columns,
              const std::map<std::string, ColumnDtype> &col_dtype) override;

  retcode setupChannel(uint16_t party_id, MpcChannel &next,
                       MpcChannel &prev) override;

private:
  bool use_mpc_div_{false};
  bool avg_result_{false};
  eMatrix<double> result;
  std::unique_ptr<MPCOperator> mpc_op_;
  uint16_t party_id_;
};

class MPCMinOrMax : public MPCStatisticsOperator {
public:
  MPCMinOrMax(const MPCStatisticsType &type) { type_ = type; }
  virtual ~MPCMinOrMax() {
    mpc_op_.reset();
  }

  retcode run(std::shared_ptr<primihub::Dataset> &dataset,
              const std::vector<std::string> &columns,
              const std::map<std::string, ColumnDtype> &col_dtype) override;

  retcode getResult(eMatrix<double> &result) override;

  retcode setupChannel(uint16_t party_id, MpcChannel &next,
                       MpcChannel &prev) override;
private:
  double minValueOfAllParty(double local_min);
  double maxValueOfAllParty(double local_max);

  template <class T1, class T2>
  void minOrMax(std::shared_ptr<T1> &array, T2 &val) {
    if (type_ == MPCStatisticsType::MAX) {
      for (int i = 0; i < array->length(); i++) {
        if (val <= array->Value(i))
          val = array->Value(i);
      }
    } else {
      for (int i = 0; i < array->length(); i++) {
        if (val >= array->Value(i))
          val = array->Value(i);
      }
    }
  }

  template <class T> void fillInitValue(T &val) {
    if (std::is_same<T, int32_t>::value) {
      if (type_ == MPCStatisticsType::MAX)
        val = std::numeric_limits<int32_t>::min();
      else
        val = std::numeric_limits<int32_t>::max();
      return;
    }

    if (std::is_same<T, int64_t>::value) {
      if (type_ == MPCStatisticsType::MAX)
        val = std::numeric_limits<int64_t>::min();
      else
        val = std::numeric_limits<int64_t>::max();
      return;
    }

    if (std::is_same<T, double>::value) {
      if (type_ == MPCStatisticsType::MAX)
        val = std::numeric_limits<double>::min();
      else
        val = std::numeric_limits<double>::max();
      return;
    }

    LOG(ERROR) << "No match type for " << typeid(val).name() << ".";
  }

  template <class T>
  void fillChunkResult(double &cur_val, T &new_val) {
    if (type_ == MPCStatisticsType::MIN) {
      if (cur_val > static_cast<double>(new_val))
        cur_val = new_val;

      return;
    }

    if (type_ == MPCStatisticsType::MAX) {
      if (cur_val < static_cast<double>(new_val))
        cur_val = new_val;
      return;
    }
  }

  eMatrix<double> mpc_result_;
  std::unique_ptr<MPCOperator> mpc_op_;
  MPCStatisticsType type_;
  uint16_t party_id_;
};
#endif  // MPC_SOCKET_CHANNEL
}; // namespace primihub

#endif
