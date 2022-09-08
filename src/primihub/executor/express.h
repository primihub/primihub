#ifndef SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_
#define SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_

#include <cstdlib>
#include <iostream>
#include <map>
#include <stack>
#include <string>

#include "src/primihub/operator/aby3_operator.h"

namespace primihub {
class LocalExpressExecutor;
class MPCExpressExecutor {
public:
  MPCExpressExecutor();
  ~MPCExpressExecutor();

  // Method group begin:

  // Run method in method group from 1 to 6 (don't change the order) will
  // evaluate a express with MPC operator.

  // Method group 1: Import all column's dtype and owner.
  void initColumnConfig(const std::string &node_id);

  int importColumnDtype(const std::string &col_name, bool is_fp64);

  int importColumnOwner(const std::string &col_name,
                        const std::string &node_id);

  // Method group 2: Import express, parse and check it.
  int importExpress(const std::string &expr);

  // Method group 3: Check whether MPC will handle double value or not.
  int resolveRunMode(void);

  // Method group 4: Import local column's value.
  void InitFeedDict(void);

  int importColumnValues(const std::string &col_name,
                         std::vector<int64_t> &col_vec);

  int importColumnValues(const std::string &col_name,
                         std::vector<double> &col_vec);

  // Method group 5: Init MPC operator.
  void initMPCRuntime(uint8_t party_id, const std::string &ip,
                      uint16_t next_port, uint16_t prev_port);

  // Method group 6: Execute express with MPC protocol.
  int runMPCEvaluate(void);

  // Method group 7: Reveal MPC result to one or more parties.
  void revealMPCResult(std::vector<uint8_t> &party, std::vector<double> &vec);

  void revealMPCResult(std::vector<uint8_t> &party, std::vector<int64_t> &vec);

  // Method group end;

  bool isFP64RunMode(void) { return this->fp64_run_; }

  void Clean(void);

  // A token means a column name and constant value string, and a TokenValue
  // saves values of a token. So:
  // 1. if token is a column name, instance of TokenValue saves column's
  // value;
  // 2. if token is a constant value string, instance of TokenValue saves
  // it's
  //    int64_t value of double value;
  // 3. if token is a sub-express, instance of TokenValue saves share value
  //    of it's result.
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

  friend class LocalExpressExecutor;

private:
  // ColumnConfig saves column's owner and it's data type.
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

    friend class LocalExpressExecutor;

  private:
    std::string node_id_;
    std::map<std::string, std::string> col_owner_;
    std::map<std::string, ColDtype> col_dtype_;
    std::map<std::string, bool> local_col_;
  };

  // FeedDict saves each local column's value.
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

    void Clean(void);

  private:
    int checkLocalColumn(const std::string &col_name);
    int setOrCheckValueCount(int64_t new_count);

    bool float_run_;
    int64_t val_count_;
    std::map<std::string, std::vector<double>> fp64_col_;
    std::map<std::string, std::vector<int64_t>> int64_col_;
    ColumnConfig *col_config_;
  };

  inline int createTokenValue(const std::string &token, TokenValue &token_val);

  inline void createTokenValue(sf64Matrix<D> *m, TokenValue &token_val);

  inline void createTokenValue(si64Matrix *m, TokenValue &token_val);

  inline void constructI64Matrix(TokenValue &token_val, i64Matrix &m);

  inline void constructFP64Matrix(TokenValue &token_val, eMatrix<double> &m);

  inline void createI64Shares(TokenValue &val, si64Matrix &sh_val);

  inline void createFP64Shares(TokenValue &val, sf64Matrix<D> &sh_val);

  inline void BeforeMPCRun(std::stack<std::string> &token_stk,
                           std::stack<TokenValue> &val_stk, TokenValue &val1,
                           TokenValue &val2, std::string &a, std::string &b);

  inline void AfterMPCRun(std::stack<std::string> &token_stk,
                          std::stack<TokenValue> &val_stk,
                          std::string &new_token, TokenValue &res);

  void runMPCAddFP64(TokenValue &val1, TokenValue &val2, TokenValue &res);

  void runMPCAddI64(TokenValue &val1, TokenValue &val2, TokenValue &res);

  void runMPCSubFP64(TokenValue &val1, TokenValue &val2, TokenValue &res);

  void runMPCSubI64(TokenValue &val1, TokenValue &val2, TokenValue &res);

  void runMPCMulFP64(TokenValue &val1, TokenValue &val2, TokenValue &res);

  void runMPCMulI64(TokenValue &val1, TokenValue &val2, TokenValue &res);

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
  std::map<std::string, TokenValue> token_val_map_;
  std::map<std::string, TokenType> token_type_map_;
  uint8_t party_id_;
};

class LocalExpressExecutor {
public:
  LocalExpressExecutor(MPCExpressExecutor *mpc_exec) {
    this->mpc_exec_ = mpc_exec;
  }

  void beforeLocalEvaluate(void);
  int runLocalEvaluate(std::vector<double> &eval_res);
  int runLocalEvaluate(std::vector<int64_t> &eval_res);
  // void afterLocalEvaluate(void);

  void createNewColumnConfig() {
    new_col_cfg =
        new MPCExpressExecutor::ColumnConfig(mpc_exec_->col_config_->node_id_);
    std::map<std::string, bool> &local_col_outside =
        mpc_exec_->col_config_->local_col_;
    for (auto &pair : local_col_outside)
      new_col_cfg->local_col_.insert(std::make_pair(pair.first, true));
    std::map<std::string, ColDtype> &local_col_dtype =
        mpc_exec_->col_config_->col_dtype_;
    for (auto &pair : local_col_dtype)
      new_col_cfg->col_dtype_.insert(std::make_pair(pair.first, pair.second));

    std::map<std::string, ColDtype> col_dtype_;
  }

  void creatNewFeedDict() {
    // old_feed = mpc_exec_->feed_dict_;
    createNewColumnConfig();
    new_feed =
        new MPCExpressExecutor::FeedDict(new_col_cfg, mpc_exec_->fp64_run_);
  }

  template <typename T>
  static void
  importColumnValues(std::map<std::string, std::vector<T>> &col_and_val) {
    for (auto &pair : col_and_val)
      new_feed->importColumnValues(pair.first, pair.second);
    return;
  }

  int createTokenValue(const std::string &token,
                       MPCExpressExecutor::TokenValue &token_val) {
    TokenType type = mpc_exec_->token_type_map_[token];
    if (type == TokenType::COLUMN) {
      if (fp64_run_) {
        if (new_feed->getColumnValues(token, &(token_val.val_union.fp64_vec))) {
          LOG(ERROR) << "Get column value with token '" << token << "' failed.";
          return -1;
        }
        token_val.type = 0;
      } else {
        if (new_feed->getColumnValues(token, &(token_val.val_union.i64_vec))) {
          LOG(ERROR) << "Get column value with token '" << token << "' failed.";
          return -1;
        }
        token_val.type = 1;
      }

    } else {
      if (fp64_run_) {
        token_val.val_union.fp64_val = std::stod(token);
        token_val.type = 2;
      } else {
        token_val.val_union.i64_val = atol(token.c_str());
        token_val.type = 3;
      }
    }
    return 0;
  }

  void init() {
    creatNewFeedDict();
    importColumnValues();
  }

private:
  using I64StackType = std::stack<std::vector<int64_t> *>;
  using FP64StackType = std::stack<std::vector<double> *>;

  std::map<std::string, MPCExpressExecutor::TokenValue> token_val_map_;

  inline void beforeLocalCalculate(std::stack<std::string> &token_stk,
                                   std::stack<TokenValue> &val_stk,
                                   std::string &a, std::string &b,
                                   TokenValue &val1, TokenValue &val2);

  inline void afterLocalCalculate(std::stack<std::string> &stk1,
                                  I64StackType &val_stk, std::string &new_token,
                                  std::vector<int64_t> *p_res);

  MPCExpressExecutor *mpc_exec_;
  // std::map<std::string, bool> local_col_;
  std::map<std::string, std::vector<int64_t> *> i64_token_val_map_;
  std::map<std::string, std::vector<double> *> fp64_token_val_map_;

  // MPCExpressExecutor::FeedDict *old_feed;
  MPCExpressExecutor::FeedDict *new_feed;
  MPCExpressExecutor::ColumnConfig *new_col_cfg;
};

}; // namespace primihub
#endif
