#ifndef SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_
#define SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_

#include <cstdlib>
#include <iostream>
#include <stack>
#include <string>

#include "src/primihub/executor/party.h"

namespace primihub {
class PartyConfig {
public:
  enum ColDtype { INT64, FP64 };

  PartyConfig(std::string node_id) { node_id_ = node_id; }
  ~PartyConfig();

  int importColumnDtype(const std::string &col_name, const ColDtype &dtype);
  int importColumnOwner(const std::string &col_name,
                        const std::string &node_id);
  int getColumnDtype(const std::string &col_name, ColDtype &dtype);
  int getColumnLocality(bool &is_local);
  int resolveLocalColumn(void);
  bool hasFP64Column(void);
  int ResolveTokenType(void);
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
  FeedDict(const std::string &node_id, const PartyConfig *party_config,
           bool float_run) {
    node_id_ = node_id;
    party_config_ = party_config;
    float_run_ = float_run;
  }

  ~FeedDict();

  int importColumnValues(const std::string &col_name,
                         std::vector<int64_t> &int64_vec);
  int importColumnValues(const std::string &col_name,
                         std::vector<double> &fp64_vec);
  template <class T>
  int getColumnValues(const std::string &col_name, std::vector<T> &col_vec);

private:
  int checkLocalColumn(const std::string &col_name);

  bool float_run_;
  std::string node_id_;
  std::map<std::string, std::vector<double>> fp64_col_;
  std::map<std::string, std::vector<int64_t>> int64_col_;
  const PartyConfig *party_config_;
};

class MPCExpressExecutor {
public:
  MPCExpressExecutor();
  ~MPCExpressExecutor();

  int importExpress(std::string expr);
  void resolveRunMode(void);
  int runMPCEvaluate(void);

  void setPartyConfig(PartyConfig *party_config) {
    this->party_config_ = party_config;
  }

  void setFeedDict(FeedDict *feed_dict) { this->feed_dict_ = feed_dict; }

private:
  union ValueUnion {
    std::vector<double> fp64_vec;
    std::vector<int64_t> i64_vec;
    int64_t i64_val;
    double fp64_val;
  };

  struct TokenValue {
    ValueUnion val;
    // type == 0: val.fp64_vec is set;
    // type == 1: val.i64_vec is set;
    // type == 2: val.fp64 is set.
    // type == 3: val.i64 is set;
    // type == 4: a remote column, set nothing.
    uint8_t type;
  };

  inline void createTokenValue(const std::string &token, TokenValue &token_val);

  inline void createI64Shares(TokenValue &val1, TokenValue val2,
                              si64Matrix &sh_val);
  inline void createFP64Shares(TokenType &val, TokenValue val2,
                               sf64Matrix<D> &sh_val);

  enum TokenType { COLUMN, VALUE };
  int ResolveTokenType(void);

  int runMPCAdd(const std::string &token1, const std::string &token2,
                sf64Matrix<D> &sh_val);
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
  PartyConfig *party_config_;
  MPCOperator *mpc_op_;
  FeedDict *feed_dict_;
  std::map<std::string, TokenType> token_type_;
};
}; // namespace primihub
#endif
