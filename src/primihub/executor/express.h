/*
 * Copyright (c) 2023 by PrimiHub
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_
#define SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_

#include <cstdlib>
#include <iostream>
#include <map>
#include <stack>
#include <string>

#include "src/primihub/operator/aby3_operator.h"

namespace primihub {

template <Decimal Dbit> class LocalExpressExecutor;

template <Decimal Dbit>
class MPCExpressExecutor {
public:
  MPCExpressExecutor();
  ~MPCExpressExecutor();

  // Method group begin:

  // Run method in method group from 1 to 6 (don't change the order) will
  // evaluate a express with MPC operator.

  // Method group 1: Import all column's dtype and owner.
  void initColumnConfig(const u32 &party_id);

  int importColumnDtype(const std::string &col_name, bool is_fp64);

  int importColumnOwner(const std::string &col_name, const u32 &party_id);

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
  void initMPCRuntime(uint32_t party_id, const std::string &next_ip,
                      const std::string &prev_ip, uint16_t next_port,
                      uint16_t prev_port);
  void initMPCRuntime(uint32_t party_id, std::shared_ptr<aby3::CommPkg> comm_pkg);
  void initMPCRuntime(uint32_t party_id, aby3::CommPkg* comm_pkg);

  // Method group 6: Execute express with MPC protocol.
  int runMPCEvaluate(void);

  // Method group 7: Reveal MPC result to one or more parties.
  void revealMPCResult(std::vector<uint32_t> &party, std::vector<double> &vec);

  void revealMPCResult(std::vector<uint32_t> &party, std::vector<int64_t> &vec);

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
    sf64Matrix<Dbit> *sh_fp64_m;
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

  template <Decimal Dbits> friend class LocalExpressExecutor;

private:
  // ColumnConfig saves column's owner and it's data type.
  class ColumnConfig {
  public:
    enum ColDtype { INT64, FP64 };

    ColumnConfig(u32 party_id) { party_id_ = party_id; }
    ~ColumnConfig();

    int importColumnDtype(const std::string &col_name, const ColDtype &dtype);
    int importColumnOwner(const std::string &col_name, const u32 &party_id);
    int getColumnDtype(const std::string &col_name, ColDtype &dtype);
    int getColumnLocality(const std::string &col_name, bool &is_local);
    int resolveLocalColumn(void);
    bool hasFP64Column(void);
    void Clean(void);

    static std::string DtypeToString(const ColDtype &dtype);

    template <Decimal Dbits> friend class LocalExpressExecutor;

  private:
    u32 party_id_;
    std::map<std::string, u32> col_owner_;
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

  inline void createTokenValue(sf64Matrix<Dbit> *m, TokenValue &token_val);

  inline void createTokenValue(si64Matrix *m, TokenValue &token_val);

  inline void constructI64Matrix(TokenValue &token_val, i64Matrix &m);

  inline void constructFP64Matrix(TokenValue &token_val, eMatrix<double> &m);

  inline void createI64Shares(TokenValue &val, si64Matrix &sh_val);

  inline void createFP64Shares(TokenValue &val, sf64Matrix<Dbit> &sh_val);

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

  void runMPCDivFP64(TokenValue &val1, TokenValue &val2, TokenValue &res);

  bool isOperator(const char op);
  bool isOperator(const std::string &op);
  int Priority(const char str);
  bool checkExpress(void);
  void parseExpress(const std::string &expr);

  bool fp64_run_;
  std::string expr_;
  std::stack<std::string> suffix_stk_;
  ColumnConfig *col_config_;
  std::unique_ptr<MPCOperator> mpc_op_;
  FeedDict *feed_dict_;
  std::map<std::string, TokenValue> token_val_map_;
  std::map<std::string, TokenType> token_type_map_;
  uint32_t party_id_;
};

template <Decimal Dbit> class LocalExpressExecutor {
public:
  LocalExpressExecutor(MPCExpressExecutor<Dbit> *mpc_exec) {
    this->mpc_exec_ = mpc_exec;
  }

  LocalExpressExecutor() { this->mpc_exec_ = nullptr; }

  ~LocalExpressExecutor();

  bool isFP64RunMode(void) { return mpc_exec_->isFP64RunMode(); }

  int runLocalEvaluate();

  void setMPCExpressExecutor(MPCExpressExecutor<Dbit> *mpc_exec) {
    this->mpc_exec_ = mpc_exec;
  }

  template <typename T>
  void init(std::map<std::string, std::vector<T>> &col_and_val) {
    createNewColumnConfig();
    creatNewFeedDict();
    importColumnValues(col_and_val);
  }
  template <typename T> void getFinalVal(std::vector<T> &final_val) {
    if (std::is_same<T, double>::value)
      for (i64 i = 0; i < final_val_double.size(); i++)
        final_val.push_back(final_val_double[i]);
    else
      for (i64 i = 0; i < final_val_int64.size(); i++)
        final_val.push_back(final_val_int64[i]);
  }

private:
  // using I64StackType = std::stack<std::vector<int64_t> *>;
  // using FP64StackType = std::stack<std::vector<double> *>;

  void createNewColumnConfig();

  void creatNewFeedDict();

  int createTokenValue(
      const std::string &token,
      typename MPCExpressExecutor<Dbit>::TokenValue &token_val);

  template <typename T>
  void importColumnValues(std::map<std::string, std::vector<T>> &col_and_val) {
    for (auto &pair : col_and_val)
      new_feed->importColumnValues(pair.first, pair.second);
    return;
  }

  inline void beforeLocalCalculate(
      std::stack<std::string> &token_stk,
      std::stack<typename MPCExpressExecutor<Dbit>::TokenValue> &val_stk,
      typename MPCExpressExecutor<Dbit>::TokenValue &val1,
      typename MPCExpressExecutor<Dbit>::TokenValue &val2, std::string &a,
      std::string &b);

  inline void
  afterLocalCalculate(std::stack<std::string> &token_stk,
                      std::stack<typename MPCExpressExecutor<Dbit>::TokenValue> &val_stk,
                      std::string &new_token,
                      typename MPCExpressExecutor<Dbit>::TokenValue &res);

  // std::map<std::string, std::vector<int64_t> *> i64_token_val_map_;
  // std::map<std::string, std::vector<double> *> fp64_token_val_map_;

  MPCExpressExecutor<Dbit> *mpc_exec_;
  std::map<std::string, typename MPCExpressExecutor<Dbit>::TokenValue> token_val_map_;
  typename MPCExpressExecutor<Dbit>::FeedDict *new_feed;
  typename MPCExpressExecutor<Dbit>::ColumnConfig *new_col_cfg;
  std::vector<double> final_val_double;
  std::vector<int64_t> final_val_int64;
};

}; // namespace primihub
#endif
