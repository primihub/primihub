#ifndef SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_
#define SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_

#include <cstdlib>
#include <iostream>
#include <map>
#include <stack>
#include <string>

#include "src/primihub/operator/aby3_operator.h"

namespace primihub {
class ColumnConfig {
public:
  enum ColDtype { INT64, FP64 };

  ColumnConfig(std::string node_id) { node_id_ = node_id; }
  ~ColumnConfig();

  int importColumnDtype(const std::string &col_name, const ColDtype &dtype);
  int importColumnOwner(const std::string &col_name,
                        const std::string &node_id);
  int getColumnDtype(const std::string &col_name, ColDtype &dtype);
  int getColumnLocality(const std::string &col_name, bool &is_local);
  int resolveLocalColumn(void);
  bool hasFP64Column(void);
  void Clean(void);

  static std::string DtypeToString(const ColDtype &dtype);

private:
  std::string node_id_;
  std::map<std::string, std::string> col_owner_;
  std::map<std::string, ColDtype> col_dtype_;
  std::map<std::string, bool> local_col_;
};

class FeedDict {
public:
  FeedDict(ColumnConfig *col_config, bool float_run) {
    col_config_ = col_config;
    float_run_ = float_run;
    val_count_ = -1;
  }

  ~FeedDict();

  int importColumnValues(const std::string &col_name,
                         std::vector<int64_t> &int64_vec);
  int importColumnValues(const std::string &col_name,
                         std::vector<double> &fp64_vec);

  int getColumnValues(const std::string &col_name,
                      std::vector<int64_t> **col_vec);
  int getColumnValues(const std::string &col_name,
                      std::vector<double> **col_vec);

  uint32_t getColumnValuesCount(void) {
    return static_cast<uint32_t>(val_count_);
  }

private:
  int checkLocalColumn(const std::string &col_name);
  int setOrCheckValueCount(int64_t new_count);

  bool float_run_;
  int64_t val_count_;
  std::map<std::string, std::vector<double>> fp64_col_;
  std::map<std::string, std::vector<int64_t>> int64_col_;
  ColumnConfig *col_config_;
};

class MPCExpressExecutor {
public:
  MPCExpressExecutor();
  ~MPCExpressExecutor();

  int importExpress(std::string expr);
  void resolveRunMode(void);
  void initMPCRuntime(uint8_t party_id, const std::string &ip,
                      uint16_t next_port, uint16_t prev_port);
  int runMPCEvaluate(void);

  void setColumnConfig(ColumnConfig *col_config) {
    this->col_config_ = col_config;
  }

  void setFeedDict(FeedDict *feed_dict) { this->feed_dict_ = feed_dict; }

  bool isFP64RunMode(void) { return this->fp64_run_; }

  enum TokenType { COLUMN, VALUE };
  union ValueUnion {
    sf64Matrix<D> *sh_fp64_m;
    si64Matrix *sh_i64_m;
    std::vector<double> *fp64_vec;
    std::vector<int64_t> *i64_vec;
    int64_t i64_val;
    double fp64_val;

    ValueUnion() {}
    ~ValueUnion() {}
  };

  struct TokenValue {
    ValueUnion val_union;
    // type == 0: val.fp64_vec is set;
    // type == 1: val.i64_vec is set;
    // type == 2: val.fp64 is set.
    // type == 3: val.i64 is set;
    // type == 4: a remote column, set nothing.
    // type == 5: a share matrix for FP64 matrix.
    // type == 6: a share matrix for I64 matrix.
    uint8_t type;

    TokenValue(){};
    ~TokenValue(){};

    std::string TypeToString(void) {
      switch (this->type) {
      case 5:
        return "share value for FP64 values";
      case 6:
        return "share value for I64 values";
      case 3:
        return "const I64 value";
      case 2:
        return "const FP64 value";
      case 1:
        return "local I64 column";
      case 0:
        return "local FP64 column";
      case 4:
        return "remote column";
      default:
        return "unknown type";
      }
    }
  };

private:
  inline void createTokenValue(const std::string &token, TokenValue &token_val);

  inline void createTokenValue(sf64Matrix<D> *m, TokenValue &token_val,
                               bool is_fp64);

  inline void constructI64Matrix(TokenValue &token_val, i64Matrix &m);

  inline void constructFP64Matrix(TokenValue &token_val, eMatrix<double> &m);

  inline void createI64Shares(TokenValue &val, si64Matrix &sh_val);

  inline void createFP64Shares(TokenValue &val, sf64Matrix<D> &sh_val);

  void runMPCAddFP64(TokenValue &val1, TokenValue &val2, TokenValue &res);

  void runMPCAddI64(TokenValue &val1, TokenValue &val2, TokenValue &res);

  int runMPCSub(const std::string &token1, const std::string &token2,
                sf64Matrix<D> &sh_val);
  int runMPCMul(const std::string &token1, const std::string &token2,
                sf64Matrix<D> &sh_val);
  int runMPCDiv(const std::string &token1, const std::string &token2,
                sf64Matrix<D> &sh_val);

  bool isOperator(const char op);
  bool isOperator(const std::string &op);
  int Priority(const char str);
  bool checkExpress(void);
  void parseExpress(const std::string &expr);

  bool fp64_run_;
  std::string expr_;
  std::stack<std::string> suffix_stk_;
  ColumnConfig *col_config_;
  MPCOperator *mpc_op_;
  FeedDict *feed_dict_;
  std::map<std::string, TokenType> token_type_;
};
}; // namespace primihub
#endif
