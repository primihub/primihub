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

private:
  int checkLocalColumn(const std::string &col_name);

  bool float_run_;
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
    uint8_t type;

    TokenValue(){};
    ~TokenValue(){};

    std::string TypeToString(void) {
      if (this->type == 4)
        return "remote column";
      else if (this->type == 3)
        return "const I64 value";
      else if (this->type == 2)
        return "const FP64 value";
      else if (this->type == 1)
        return "local I64 column";
      else
        return "local FP64 column";
    }
  };

private:
  inline void createTokenValue(const std::string &token, TokenValue &token_val);

  inline void constructI64Matrix(TokenValue &token_val, i64Matrix &m);

  inline void constructFP64Matrix(TokenValue &token_val, eMatrix<double> &m);

  inline void createI64Shares(TokenValue &val1, TokenValue &val2,
                              si64Matrix &sh_val1, si64Matrix &sh_val2);

  inline void createFP64Shares(TokenValue &val1, TokenValue &val2,
                               sf64Matrix<D> &sh_val1, sf64Matrix<D> &sh_val2);

  void runMPCAdd(const std::string &token1, const std::string &token2,
                 sf64Matrix<D> &sh_val);
  void runMPCAdd(const std::string &token1, const std::string &token2,
                 si64Matrix &sh_val);

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
