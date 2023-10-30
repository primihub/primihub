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

#include <glog/logging.h>
#include <sstream>
#include <time.h>
#include <string>

#include "src/primihub/executor/express.h"

#define TokenValue typename primihub::MPCExpressExecutor<Dbit>::TokenValue

namespace primihub {
// Implement of class ColumnConfig.
template <Decimal Dbit>
std::string
MPCExpressExecutor<Dbit>::ColumnConfig::DtypeToString(const ColDtype &dtype) {
  if (dtype == ColDtype::FP64)
    return std::string("FP64");
  else
    return std::string("I64");
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::ColumnConfig::importColumnDtype(
    const std::string &col_name, const ColDtype &dtype) {
  auto iter = col_dtype_.find(col_name);
  if (iter != col_dtype_.end()) {
    LOG(ERROR) << "Column " << col_name
               << "'s dtype attr is imported, value is " << DtypeToString(dtype)
               << ".";
    return -1;
  }

  col_dtype_.insert(std::make_pair(col_name, dtype));
  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::ColumnConfig::importColumnOwner(
    const std::string &col_name, const u32 &party_id) {
  auto iter = col_owner_.find(col_name);
  if (iter != col_owner_.end()) {
    LOG(ERROR) << "Column " << col_name
               << "'s owner attr is imported, value is " << iter->second << ".";
    return -1;
  }

  col_owner_.insert(std::make_pair(col_name, party_id));
  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::ColumnConfig::getColumnDtype(
    const std::string &col_name, ColDtype &dtype) {
  auto iter = col_dtype_.find(col_name);
  if (iter == col_dtype_.end()) {
    LOG(ERROR) << "Can't find dtype attr for column " << col_name << ".";
    return -1;
  }

  dtype = iter->second;
  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::ColumnConfig::resolveLocalColumn(void) {
  if (col_dtype_.size() != col_owner_.size()) {
    LOG(ERROR) << "Count of owner attr and dtype attr must be the same.";
    return -1;
  }

  for (auto iter = col_owner_.begin(); iter != col_owner_.end(); iter++) {
    if (col_dtype_.find(iter->first) == col_dtype_.end()) {
      LOG(ERROR) << "Import column " << iter->first
                 << "'s owner attr, but don't import it's dtype attr.";
      return -2;
    }
  }

  for (auto iter = col_owner_.begin(); iter != col_owner_.end(); iter++) {
    if (iter->second == party_id_)
      local_col_.insert(std::make_pair(iter->first, true));
    else
      local_col_.insert(std::make_pair(iter->first, false));
  }

  {
    uint8_t count = 0;
    LOG(INFO) << "Dump column locality:";
    for (auto iter = local_col_.begin(); iter != local_col_.end(); iter++) {
      count++;
      if (iter->second == true)
        LOG(INFO) << "Column " << iter->first << ": local;";
      else
        LOG(INFO) << "Column " << iter->first << ": remote;";
    }

    if (count < 10)
      LOG(INFO) << "Dump finish, dump count " << static_cast<char>(count + '0')
                << ".";
    else
      LOG(INFO) << "Dump finish, dump count " << count << ".";
  }

  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::ColumnConfig::getColumnLocality(
    const std::string &col_name, bool &is_local) {
  auto iter = local_col_.find(col_name);
  if (iter == local_col_.end()) {
    LOG(ERROR) << "Can't find column locality by column name " << col_name
               << ".";
    return -1;
  }

  is_local = iter->second;
  return 0;
}

template <Decimal Dbit>
bool MPCExpressExecutor<Dbit>::ColumnConfig::hasFP64Column(void) {
  bool fp64_col = false;
  for (auto iter = col_dtype_.begin(); iter != col_dtype_.end(); iter++) {
    if (iter->second == ColDtype::FP64) {
      fp64_col = true;
      break;
    }
  }

  return fp64_col;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::ColumnConfig::Clean(void) {
  col_owner_.clear();
  col_dtype_.clear();
  local_col_.clear();
  // node_id_ = "";
}

template <Decimal Dbit>
MPCExpressExecutor<Dbit>::ColumnConfig::~ColumnConfig() {
  Clean();
}

// Implement of class FeedDict.
template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::FeedDict::checkLocalColumn(
    const std::string &col_name) {
  bool is_local = false;
  int ret = col_config_->getColumnLocality(col_name, is_local);
  if (ret) {
    LOG(ERROR) << "Get column locality by column name " << col_name
               << " failed.";
    return -1;
  }

  if (is_local == false)
    return 0;
  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::FeedDict::setOrCheckValueCount(
    int64_t new_count) {
  if (val_count_ == -1)
    val_count_ = new_count;

  if (val_count_ != new_count)
    return -1;

  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::FeedDict::importColumnValues(
    const std::string &col_name, std::vector<int64_t> &int64_vec) {
  {
    bool is_local = false;
    int ret = col_config_->getColumnLocality(col_name, is_local);
    if (ret < 0) {
      LOG(ERROR) << "Get column " << col_name << "'s locality failed.";
      return -1;
    }

    if (is_local == false) {
      LOG(ERROR) << "Column " << col_name << " isn't a local column.";
      return -1;
    }
  }

  typename ColumnConfig::ColDtype dtype;
  int ret = col_config_->getColumnDtype(col_name, dtype);
  if (ret) {
    LOG(ERROR) << "Get column " << col_name << "'s dtype failed.";
    return -2;
  }

  if (dtype != ColumnConfig::ColDtype::INT64) {
    LOG(ERROR) << "Try to import column values type of which is I64, but type "
                  "of it is FP64 in column config.";
    return -2;
  }

  if (setOrCheckValueCount(static_cast<int64_t>(int64_vec.size()))) {
    LOG(ERROR) << "Column " << col_name << " has " << int64_vec.size()
               << " value, but column imported before has " << val_count_
               << " value.";
    return -2;
  }

  if (float_run_ == true) {
    LOG(INFO) << "Convert int64 value to fp64 value due to fp64 run mode.";
    std::vector<double> fp64_vec;
    for (auto v : int64_vec)
      fp64_vec.emplace_back(static_cast<double>(v));

    int ret = importColumnValues(col_name, fp64_vec);
    if (ret) {
      LOG(ERROR) << "Import values of column " << col_name << " failed.";
      return -3;
    }

    fp64_col_.insert(std::make_pair(col_name, std::move(fp64_vec)));
    int64_vec.clear();
    return 0;
  }

  if (int64_col_.find(col_name) != int64_col_.end()) {
    LOG(ERROR) << "Value of column " << col_name << " is already imported.";
    return -4;
  }

  int64_col_.insert(std::make_pair(col_name, std::move(int64_vec)));
  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::FeedDict::importColumnValues(
    const std::string &col_name, std::vector<double> &fp64_vec) {
  {
    bool is_local = false;
    int ret = col_config_->getColumnLocality(col_name, is_local);
    if (ret < 0) {
      LOG(ERROR) << "Get column " << col_name << "'s locality failed.";
      return -1;
    }

    if (is_local == false) {
      LOG(ERROR) << "Column " << col_name << " isn't a local column.";
      return -1;
    }
  }

  if (setOrCheckValueCount(static_cast<int64_t>(fp64_vec.size()))) {
    LOG(ERROR) << "Column " << col_name << " has " << fp64_vec.size()
               << " value, but column imported before has " << val_count_
               << " value.";
    return -2;
  }

  if (float_run_ == false) {
    LOG(ERROR) << "Current run mode is int64, but import values type of which "
                  "is fp64. This should be a bug, fix it.";
    return -1;
  }

  if (fp64_col_.find(col_name) != fp64_col_.end()) {
    LOG(ERROR) << "Value of column " << col_name << " is already imported.";
    return -2;
  }

  fp64_col_.insert(std::make_pair(col_name, std::move(fp64_vec)));
  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::FeedDict::getColumnValues(
    const std::string &col_name, std::vector<int64_t> **p_col_vec) {
  {
    bool is_local = false;
    int ret = col_config_->getColumnLocality(col_name, is_local);
    if (ret < 0) {
      LOG(ERROR) << "Get column " << col_name << "'s locality failed.";
      return -1;
    }

    if (is_local == false) {
      LOG(ERROR) << "Column " << col_name << " isn't a local column.";
      return -1;
    }
  }

  if (float_run_ == true) {
    LOG(ERROR) << "Current run mode is FP64 but want to get I64 values.";
    return -1;
  }

  auto iter = int64_col_.find(col_name);
  if (iter == int64_col_.end()) {
    LOG(ERROR) << "Can't find values by column name " << col_name << ".";
    return -2;
  }

  *p_col_vec = &(iter->second);
  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::FeedDict::getColumnValues(
    const std::string &col_name, std::vector<double> **p_col_vec) {
  {
    bool is_local = false;
    int ret = col_config_->getColumnLocality(col_name, is_local);
    if (ret < 0) {
      LOG(ERROR) << "Get column " << col_name << "'s locality failed.";
      return -1;
    }

    if (is_local == false) {
      LOG(ERROR) << "Column " << col_name << " isn't a local column.";
      return -1;
    }
  }

  if (float_run_ == false) {
    LOG(ERROR) << "Current run mode is I64 but want to get FP64 values.";
    return -1;
  }

  auto iter = fp64_col_.find(col_name);
  if (iter == fp64_col_.end()) {
    LOG(ERROR) << "Can't find values by column name " << col_name << ".";
    return -2;
  }

  *p_col_vec = &(iter->second);
  return 0;
}

template <Decimal Dbit> void MPCExpressExecutor<Dbit>::FeedDict::Clean(void) {
  this->float_run_ = false;
  this->fp64_col_.clear();
  this->int64_col_.clear();
  this->col_config_ = nullptr;
}

template <Decimal Dbit> MPCExpressExecutor<Dbit>::FeedDict::~FeedDict() {
  Clean();
}

// Implement of class MPCExpressExecutor<Dbit>.
template <Decimal Dbit> MPCExpressExecutor<Dbit>::MPCExpressExecutor() {
  col_config_ = nullptr;
  feed_dict_ = nullptr;
}

template <Decimal Dbit>
bool MPCExpressExecutor<Dbit>::isOperator(const char op) {
  if (op == '+' || op == '-' || op == '*' || op == '/')
    return true;
  return false;
}

template <Decimal Dbit>
bool MPCExpressExecutor<Dbit>::isOperator(const std::string &op) {
  if (op == "+")
    return true;
  else if (op == "-")
    return true;
  else if (op == "*")
    return true;
  else if (op == "/")
    return true;
  else
    return false;
}

template <Decimal Dbit> int MPCExpressExecutor<Dbit>::Priority(const char str) {
  int priv = 0;
  switch (str) {
  case '+':
    priv = 1;
    break;
  case '-':
    priv = 1;
    break;
  case '*':
    priv = 2;
    break;
  case '/':
    priv = 2;
    break;
  }

  return priv;
}

template <Decimal Dbit> bool MPCExpressExecutor<Dbit>::checkExpress(void) {
  std::vector<std::string> suffix_vec;
  std::stack<std::string> tmp_stk = suffix_stk_;

  while (!tmp_stk.empty()) {
    suffix_vec.push_back(tmp_stk.top());
    tmp_stk.pop();
  }

  // "(" and ")" can't appears in suffix express.
  for (auto str : suffix_vec) {
    if (str == "(" || str == ")") {
      LOG(ERROR) << "Illegal express found, too many bracket in express.";
      return false;
    }
  }

  // Number mismatch between operator and others.
  uint16_t num_op = 0;
  for (auto str : suffix_vec) {
    if (isOperator(str))
      num_op++;
  }

  if ((suffix_vec.size() - num_op) != static_cast<size_t>(num_op + 1)) {
    LOG(ERROR) << "Illegal express found, too many operator"
               << " or column in express.";
    return false;
  }

  for (auto str : suffix_vec) {
    if (str.find("(") != std::string::npos) {
      LOG(ERROR) << "Illegal express found, lack operator in express.";
      return false;
    }

    if (str.find(")") != std::string::npos) {
      LOG(ERROR) << "Illegal express found, lack operator in express.";
      return false;
    }
  }

  return true;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::parseExpress(const std::string &expr) {
  std::string temp;
  std::string op;
  std::string suffix;
  std::stack<std::string> stk2;

  for (size_t i = 0; i <= expr.length(); i++) {
    if ((!temp.empty()) && (i == expr.length())) {
      stk2.push(temp);
      temp.clear();
      continue;
    }

    if (i == expr.length()) {
      continue;
    }

    if (expr[i] == '-') {
      // expr[i] maybe a negative sign.
      if (i == 0) {
        temp += expr[i];
        i++;
        while (!isOperator(expr[i])) {
          temp += expr[i];
          i++;
        }

        i--;
        continue;
      } else if (expr[i - 1] == '(') {
        // Found negative value.
        while (expr[i] != ')') {
          temp += expr[i];
          i++;
        }

        i--;
        continue;
      }
    }

    if (isOperator(expr[i])) {
      // expr[i] is a operator.
      if (!temp.empty()) {
        stk2.push(temp);
        temp.clear();
      }

      while (1) {
        if (suffix_stk_.empty() || suffix_stk_.top().c_str()[0] == '(') {
          op = expr[i];
          suffix_stk_.push(op);
          break;
        } else if (Priority(expr[i]) > Priority(suffix_stk_.top().c_str()[0])) {
          op = expr[i];
          suffix_stk_.push(op);
          break;
        } else {
          op = suffix_stk_.top();
          suffix_stk_.pop();
          stk2.push(op);
        }
      }
    } else if (expr[i] == '(') {
      // expr[i] is '('.
      op = expr[i];
      suffix_stk_.push(op);
    } else if (expr[i] == ')') {
      // expr[i] is ')'.
      while (!suffix_stk_.empty() && suffix_stk_.top().c_str()[0] != '(') {
        if (!temp.empty()) {
          stk2.push(temp);
          temp.clear();
        }
        op = suffix_stk_.top();
        suffix_stk_.pop();
        stk2.push(op);
      }

      if (!suffix_stk_.empty())
        suffix_stk_.pop();
    } else {
      // str[i] is a character of column name or a constant.
      temp += expr[i];
    }
  }

  while (!stk2.empty()) {
    temp = stk2.top();
    suffix_stk_.push(temp);
    stk2.pop();
  }

  std::stack<std::string> tmp;
  tmp = suffix_stk_;
  while (!tmp.empty()) {
    suffix = suffix + " " + tmp.top();
    tmp.pop();
  }

  LOG(INFO) << "Infix express is : " << expr << ".";
  LOG(INFO) << "Suffix express is : " << suffix << ".";

  return;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::initColumnConfig(const u32 &party_id) {
  col_config_ = new ColumnConfig(party_id);
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::importColumnDtype(const std::string &col_name,
                                                bool is_fp64) {
  if (is_fp64)
    return col_config_->importColumnDtype(col_name,
                                          ColumnConfig::ColDtype::FP64);
  else
    return col_config_->importColumnDtype(col_name,
                                          ColumnConfig::ColDtype::INT64);

  if (is_fp64)
    LOG(INFO) << "Column " << col_name << "'s dtype is "
              << " FP64.";
  else
    LOG(INFO) << "Column " << col_name << "'s dtype is "
              << " I64.";

  return 0;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::importColumnOwner(const std::string &col_name,
                                                const u32 &party_id) {
  LOG(INFO) << "Column " << col_name << " belong to " << party_id << ".";
  return col_config_->importColumnOwner(col_name, party_id);
}

template <Decimal Dbit> void MPCExpressExecutor<Dbit>::InitFeedDict(void) {
  if (!feed_dict_)
    feed_dict_ = new FeedDict(col_config_, fp64_run_);
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::importColumnValues(
    const std::string &col_name, std::vector<int64_t> &val_vec) {
  return feed_dict_->importColumnValues(col_name, val_vec);
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::importColumnValues(const std::string &col_name,
                                                 std::vector<double> &val_vec) {
  return feed_dict_->importColumnValues(col_name, val_vec);
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::importExpress(const std::string &expr) {
  parseExpress(expr);
  bool ret = checkExpress();
  if (ret == false) {
    LOG(ERROR) << "Import express '" << expr << "' failed.";
    return -1;
  }
  expr_ = expr;
  return 0;
}

template <Decimal Dbit> int MPCExpressExecutor<Dbit>::resolveRunMode(void) {
  std::stack<std::string> tmp_suffix = suffix_stk_;
  bool has_div_op = false;
  bool has_fp64_val = false;

  int ret = col_config_->resolveLocalColumn();
  if (ret) {
    LOG(ERROR) << "Find out local column failed.";
    return -1;
  }

  while (!tmp_suffix.empty()) {
    std::string token = tmp_suffix.top();
    tmp_suffix.pop();

    if (isOperator(token)) {
      // Token is an operator.
      if (token == "/")
        has_div_op = true;
      continue;
    }

    typename ColumnConfig::ColDtype dtype;
    if (!col_config_->getColumnDtype(token, dtype)) {
      // Token is a column name.
      token_type_map_.insert(std::make_pair(token, TokenType::COLUMN));
      continue;
    }

    // Token is a number.
    token_type_map_.insert(std::make_pair(token, TokenType::VALUE));
    if (token.find(".") != std::string::npos)
      has_fp64_val = true;
  }

  bool has_fp64_col = col_config_->hasFP64Column();

  if (has_div_op | has_fp64_col | has_fp64_val)
    fp64_run_ = true;
  else
    fp64_run_ = false;

  if (fp64_run_)
    LOG(INFO) << "MPC run in FP64 mode.";
  else
    LOG(INFO) << "MPC run in I64 mode.";

  return 0;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::initMPCRuntime(uint32_t party_id,
                                              const std::string &next_ip,
                                              const std::string &prev_ip,
                                              uint16_t next_port,
                                              uint16_t prev_port) {
  std::string next_name;
  std::string prev_name;

  if (party_id == 0) {
    next_name = "01";
    prev_name = "02";
  } else if (party_id == 1) {
    next_name = "12";
    prev_name = "01";
  } else if (party_id == 2) {
    next_name = "02";
    prev_name = "12";
  }

  mpc_op_ = std::make_unique<MPCOperator>(party_id, next_name, prev_name);
  mpc_op_->setup(next_ip, prev_ip, next_port, prev_port);

  party_id_ = party_id;
  return;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::initMPCRuntime(
    uint32_t party_id,
    std::shared_ptr<aby3::CommPkg> comm_pkg) {
  mpc_op_ = std::make_unique<MPCOperator>(party_id, "fake_next", "fake_prev");
  mpc_op_->setup(std::move(comm_pkg));
  party_id_ = party_id;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::initMPCRuntime(uint32_t party_id,
                                              aby3::CommPkg* comm_pkg) {
  mpc_op_ = std::make_unique<MPCOperator>(party_id, "fake_next", "fake_prev");
  mpc_op_->setup(comm_pkg);
  party_id_ = party_id;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::constructI64Matrix(TokenValue &val,
                                                  i64Matrix &m) {
  if (val.type == 3) {
    m.resize(1, 1);
    m(0, 0) = val.val_union.fp64_val;
  } else {
    std::vector<int64_t> *p_vec;
    p_vec = val.val_union.i64_vec;
    m.resize(p_vec->size(), 1);
    for (size_t i = 0; i < p_vec->size(); i++) {
      m(i, 0) = (*p_vec)[i];
    }
  }

  return;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::createI64Shares(TokenValue &val,
                                               si64Matrix &sh_val) {
  if (val.type == 4) {
    mpc_op_->createShares(sh_val);
  } else {
    i64Matrix m;
    constructI64Matrix(val, m);
    mpc_op_->createShares(m, sh_val);
  }
  return;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::constructFP64Matrix(TokenValue &val,
                                                   eMatrix<double> &m) {
  if (val.type == 2) {
    m.resize(1, 1);
    m(0, 0) = val.val_union.i64_val;
  } else {
    std::vector<double> *p_vec = val.val_union.fp64_vec;
    m.resize(p_vec->size(), 1);
    for (size_t i = 0; i < p_vec->size(); i++) {
      m(i, 0) = (*p_vec)[i];
    }
  }

  return;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::createFP64Shares(TokenValue &val,
                                                sf64Matrix<Dbit> &sh_val) {
  if (val.type == 4) {
    mpc_op_->createShares<Dbit>(sh_val);
  } else {
    eMatrix<double> m;
    constructFP64Matrix(val, m);
    mpc_op_->createShares<Dbit>(m, sh_val);
  }
  return;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::createTokenValue(sf64Matrix<Dbit> *m,
                                                TokenValue &v) {
  v.val_union.sh_fp64_m = m;
  v.type = 5;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::createTokenValue(si64Matrix *m, TokenValue &v) {
  v.val_union.sh_i64_m = m;
  v.type = 6;
}

template <Decimal Dbit>
int MPCExpressExecutor<Dbit>::createTokenValue(const std::string &token,
                                               TokenValue &token_val) {
  TokenType type = token_type_map_[token];
  if (type == TokenType::COLUMN) {
    // Token is a column name, a remote column or a local column.
    bool errors = false;
    do {
      bool is_local = false;
      if (col_config_->getColumnLocality(token, is_local)) {
        errors = true;
        LOG(ERROR) << "Get column locality with token '" << token
                   << "' failed.";
        break;
      }

      if (is_local == false) {
        // Column is a remote column.
        token_val.type = 4;
        LOG(INFO) << "Construct TokenValue instance for '" << token
                  << "', type '" << token_val.TypeToString() << "'.";
        return 0;
      }

      // Column is a local column.
      if (fp64_run_) {
        if (feed_dict_->getColumnValues(token,
                                        &(token_val.val_union.fp64_vec))) {
          errors = true;
          LOG(ERROR) << "Get column value with token '" << token << "' failed.";
          break;
        }

        token_val.type = 0;
      } else {
        if (feed_dict_->getColumnValues(token,
                                        &(token_val.val_union.i64_vec))) {
          errors = true;
          LOG(ERROR) << "Get column value with token '" << token << "' failed.";
          break;
        }

        token_val.type = 1;
      }
    } while (0);

    if (errors) {
      LOG(ERROR) << "Error occurs during create token value for local column '"
                 << token << "'.";
      return -2;
    }
  } else {
    // Token is a number, maybe a float number.
    if (fp64_run_) {
      token_val.val_union.fp64_val = std::stod(token);
      token_val.type = 2;
    } else {
      token_val.val_union.i64_val = atol(token.c_str());
      token_val.type = 3;
    }
  }

  LOG(INFO) << "Construct TokenValue instance for '" << token << "', type '"
            << token_val.TypeToString() << "'.";
  return 0;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::runMPCAddFP64(TokenValue &val1, TokenValue &val2,
                                             TokenValue &res) {
  sf64Matrix<Dbit> sh_val1, sh_val2;
  sf64Matrix<Dbit> *p_sh_val1 = nullptr;
  sf64Matrix<Dbit> *p_sh_val2 = nullptr;
  f64<Dbit> constfixed;

  uint32_t val_count = feed_dict_->getColumnValuesCount();

  if (val1.type == 0 || val1.type == 4) {
    sh_val1.resize(val_count, 1);
    createFP64Shares(val1, sh_val1);
    p_sh_val1 = &sh_val1;
  } else if (val1.type == 2) {
    constfixed = val1.val_union.fp64_val;
  } else {
    p_sh_val1 = val1.val_union.sh_fp64_m;
  }

  if (val2.type == 0 || val2.type == 4) {
    sh_val2.resize(val_count, 1);
    createFP64Shares(val2, sh_val2);
    p_sh_val2 = &sh_val2;
  } else if (val2.type == 2) {
    constfixed = val2.val_union.fp64_val;
  } else {
    p_sh_val2 = val2.val_union.sh_fp64_m;
  }

  sf64Matrix<Dbit> *sh_res = new sf64Matrix<Dbit>(val_count, 1);
  if (val1.type != 2 && val2.type != 2) {
    std::vector<sf64Matrix<Dbit>> sh_val_vec;
    sh_val_vec.emplace_back(*p_sh_val1);
    sh_val_vec.emplace_back(*p_sh_val2);

    *sh_res = mpc_op_->MPC_Add(sh_val_vec);
  } else {
    if (val1.type == 2) {
      *sh_res = mpc_op_->MPC_Add_Const(constfixed, *p_sh_val2);
    } else {
      *sh_res = mpc_op_->MPC_Add_Const(constfixed, *p_sh_val1);
    }
  }
  createTokenValue(sh_res, res);
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::runMPCAddI64(TokenValue &val1, TokenValue &val2,
                                            TokenValue &res) {
  si64Matrix sh_val1, sh_val2;
  si64Matrix *p_sh_val1 = nullptr;
  si64Matrix *p_sh_val2 = nullptr;
  i64 constInt;

  uint32_t val_count = feed_dict_->getColumnValuesCount();
  if (val1.type == 1 || val1.type == 4) {
    sh_val1.resize(val_count, 1);
    createI64Shares(val1, sh_val1);
    p_sh_val1 = &sh_val1;
  } else if (val1.type == 3) {
    constInt = val1.val_union.i64_val;
  } else {
    p_sh_val1 = val1.val_union.sh_i64_m;
  }

  if (val2.type == 1 || val2.type == 4) {
    sh_val2.resize(val_count, 1);
    createI64Shares(val2, sh_val2);
    p_sh_val2 = &sh_val2;
  } else if (val2.type == 3) {
    constInt = val2.val_union.i64_val;
  } else {
    p_sh_val2 = val2.val_union.sh_i64_m;
  }

  si64Matrix *sh_res = new si64Matrix(val_count, 1);

  if (val1.type != 3 && val2.type != 3) {
    std::vector<si64Matrix> sh_val_vec;
    sh_val_vec.emplace_back(*p_sh_val1);
    sh_val_vec.emplace_back(*p_sh_val2);
    *sh_res = mpc_op_->MPC_Add(sh_val_vec);
  } else {
    if (val1.type == 3)
      *sh_res = mpc_op_->MPC_Add_Const(constInt, *p_sh_val2);
    else
      *sh_res = mpc_op_->MPC_Add_Const(constInt, *p_sh_val1);
  }

  createTokenValue(sh_res, res);
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::runMPCSubFP64(TokenValue &val1, TokenValue &val2,
                                             TokenValue &res) {
  sf64Matrix<Dbit> sh_val1, sh_val2;
  sf64Matrix<Dbit> *p_sh_val1 = nullptr;
  sf64Matrix<Dbit> *p_sh_val2 = nullptr;
  f64<Dbit> constfixed;

  uint32_t val_count = feed_dict_->getColumnValuesCount();

  if (val1.type == 0 || val1.type == 4) {
    sh_val1.resize(val_count, 1);
    createFP64Shares(val1, sh_val1);
    p_sh_val1 = &sh_val1;
  } else if (val1.type == 2) {
    constfixed = val1.val_union.fp64_val;
  } else {
    p_sh_val1 = val1.val_union.sh_fp64_m;
  }

  if (val2.type == 0 || val2.type == 4) {
    sh_val2.resize(val_count, 1);
    createFP64Shares(val2, sh_val2);
    p_sh_val2 = &sh_val2;
  } else if (val2.type == 2) {
    constfixed = val2.val_union.fp64_val;
  } else {
    p_sh_val2 = val2.val_union.sh_fp64_m;
  }

  sf64Matrix<Dbit> *sh_res = new sf64Matrix<Dbit>(val_count, 1);
  if (val1.type != 2 && val2.type != 2) {
    std::vector<sf64Matrix<Dbit>> sh_val_vec;
    sh_val_vec.emplace_back(*p_sh_val2);
    *sh_res = mpc_op_->MPC_Sub(*p_sh_val1, sh_val_vec);
  } else {
    if (val1.type == 2) {
      *sh_res = mpc_op_->MPC_Sub_Const(constfixed, *p_sh_val2, false);
    } else
      *sh_res = mpc_op_->MPC_Sub_Const(constfixed, *p_sh_val1, true);
  }
  createTokenValue(sh_res, res);
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::runMPCSubI64(TokenValue &val1, TokenValue &val2,
                                            TokenValue &res) {
  si64Matrix sh_val1, sh_val2;
  si64Matrix *p_sh_val1 = nullptr;
  si64Matrix *p_sh_val2 = nullptr;
  i64 constInt;

  uint32_t val_count = feed_dict_->getColumnValuesCount();

  if (val1.type == 1 || val1.type == 4) {
    sh_val1.resize(val_count, 1);
    createI64Shares(val1, sh_val1);
    p_sh_val1 = &sh_val1;
  } else if (val1.type == 3) {
    constInt = val1.val_union.i64_val;
  } else {
    p_sh_val1 = val1.val_union.sh_i64_m;
  }

  if (val2.type == 1 || val2.type == 4) {
    sh_val2.resize(val_count, 1);
    createI64Shares(val2, sh_val2);
    p_sh_val2 = &sh_val2;
  } else if (val2.type == 3) {
    constInt = val2.val_union.i64_val;
  } else {
    p_sh_val2 = val2.val_union.sh_i64_m;
  }

  si64Matrix *sh_res = new si64Matrix(val_count, 1);
  if (val1.type != 3 && val2.type != 3) {
    std::vector<si64Matrix> sh_val_vec;
    sh_val_vec.emplace_back(*p_sh_val2);
    *sh_res = mpc_op_->MPC_Sub(*p_sh_val1, sh_val_vec);
  } else {
    if (val1.type == 3)
      *sh_res = mpc_op_->MPC_Sub_Const(constInt, *p_sh_val2, false);
    else {
      // LOG(INFO) << constInt;
      // LOG(INFO) << (*p_sh_val1)[0](0, 0);
      // LOG(INFO) << (*p_sh_val1)[1](0, 0);
      *sh_res = mpc_op_->MPC_Sub_Const(constInt, *p_sh_val1, true);
    }
  }
  createTokenValue(sh_res, res);
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::runMPCMulFP64(TokenValue &val1, TokenValue &val2,
                                             TokenValue &res) {
  sf64Matrix<Dbit> sh_val1, sh_val2;
  sf64Matrix<Dbit> *p_sh_val1 = nullptr;
  sf64Matrix<Dbit> *p_sh_val2 = nullptr;
  f64<Dbit> constfixed;
  uint32_t val_count = feed_dict_->getColumnValuesCount();

  if (val1.type == 0 || val1.type == 4) {
    sh_val1.resize(val_count, 1);
    createFP64Shares(val1, sh_val1);
    p_sh_val1 = &sh_val1;
  } else if (val1.type == 2) {
    constfixed = val1.val_union.fp64_val;
  } else {
    p_sh_val1 = val1.val_union.sh_fp64_m;
  }

  if (val2.type == 0 || val2.type == 4) {
    sh_val2.resize(val_count, 1);
    createFP64Shares(val2, sh_val2);
    p_sh_val2 = &sh_val2;
  } else if (val2.type == 2) {
    constfixed = val2.val_union.fp64_val;
  } else {
    p_sh_val2 = val2.val_union.sh_fp64_m;
  }
  sf64Matrix<Dbit> *sh_res = new sf64Matrix<Dbit>(val_count, 1);
  if (val1.type != 2 && val2.type != 2) {
    *sh_res = mpc_op_->MPC_Dot_Mul(*p_sh_val1, *p_sh_val2);
  } else {
    if (val1.type == 2)
      *sh_res = mpc_op_->MPC_Mul_Const(constfixed, *p_sh_val2);
    else
      *sh_res = mpc_op_->MPC_Mul_Const(constfixed, *p_sh_val1);
  }
  createTokenValue(sh_res, res);
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::runMPCMulI64(TokenValue &val1, TokenValue &val2,
                                            TokenValue &res) {
  si64Matrix sh_val1, sh_val2;
  si64Matrix *p_sh_val1 = nullptr;
  si64Matrix *p_sh_val2 = nullptr;
  i64 constInt;

  uint32_t val_count = feed_dict_->getColumnValuesCount();


  if (val1.type == 1 || val1.type == 4) {
    sh_val1.resize(val_count, 1);
    createI64Shares(val1, sh_val1);
    p_sh_val1 = &sh_val1;
  } else if (val1.type == 3) {
    constInt = val1.val_union.i64_val;
  } else {
    p_sh_val1 = val1.val_union.sh_i64_m;
  }

  if (val2.type == 1 || val2.type == 4) {
    sh_val2.resize(val_count, 1);
    createI64Shares(val2, sh_val2);
    p_sh_val2 = &sh_val2;
  } else if (val2.type == 3) {
    constInt = val2.val_union.i64_val;
  } else {
    p_sh_val2 = val2.val_union.sh_i64_m;
  }



  si64Matrix *sh_res = new si64Matrix(val_count, 1);

  if (val1.type != 3 && val2.type != 3) {
    *sh_res = mpc_op_->MPC_Dot_Mul(*p_sh_val1, *p_sh_val2);
  } else {
    if (val1.type == 3)
      *sh_res = mpc_op_->MPC_Mul_Const(constInt, *p_sh_val2);
    else
      *sh_res = mpc_op_->MPC_Mul_Const(constInt, *p_sh_val1);
  }


  createTokenValue(sh_res, res);
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::runMPCDivFP64(TokenValue &val1, TokenValue &val2,
                                             TokenValue &res) {

  sf64Matrix<Dbit> sh_val1, sh_val2;
  sf64Matrix<Dbit> *p_sh_val1 = nullptr;
  sf64Matrix<Dbit> *p_sh_val2 = nullptr;
  f64<Dbit> constfixed;
  eMatrix<double> temp_f64Matrix;

  uint32_t val_count = feed_dict_->getColumnValuesCount();
  if (val1.type == 0 || val1.type == 4) {
    sh_val1.resize(val_count, 1);
    createFP64Shares(val1, sh_val1);
    p_sh_val1 = &sh_val1;
  } else if (val1.type == 2) {
    temp_f64Matrix.resize(val_count, 1);
    sh_val1.resize(val_count, 1);
    for (u64 i = 0; i < val_count; i++)
      temp_f64Matrix(i, 0) = val1.val_union.fp64_val;
    // createshares
    if (party_id_ == 0)
      mpc_op_->createShares<Dbit>(temp_f64Matrix, sh_val1);
    else
      mpc_op_->createShares<Dbit>(sh_val1);
    p_sh_val1 = &sh_val1;
  } else {
    p_sh_val1 = val1.val_union.sh_fp64_m;
  }
  if (val2.type == 0 || val2.type == 4) {
    sh_val2.resize(val_count, 1);
    createFP64Shares(val2, sh_val2);
    p_sh_val2 = &sh_val2;
  } else if (val2.type == 2) {
    constfixed = val2.val_union.fp64_val;
  } else {
    p_sh_val2 = val2.val_union.sh_fp64_m;
  }
  sf64Matrix<Dbit> *sh_res = new sf64Matrix<Dbit>(val_count, 1);
  if (val1.type != 2 && val2.type != 2) {
    *sh_res = mpc_op_->MPC_Div(*p_sh_val1, *p_sh_val2);
  } else {
    if (val1.type == 2) {
      *sh_res = mpc_op_->MPC_Div(*p_sh_val1, *p_sh_val2);
    } else {
      constfixed = 1.0 / static_cast<double>(constfixed);
      *sh_res = mpc_op_->MPC_Mul_Const(constfixed, *p_sh_val1);
    }
  }
  createTokenValue(sh_res, res);
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::BeforeMPCRun(std::stack<std::string> &token_stk,
                                            std::stack<TokenValue> &val_stk,
                                            TokenValue &val1, TokenValue &val2,
                                            std::string &a, std::string &b) {
  b = token_stk.top();
  token_stk.pop();
  a = token_stk.top();
  token_stk.pop();

  val2 = val_stk.top();
  val_stk.pop();
  val1 = val_stk.top();
  val_stk.pop();

  suffix_stk_.pop();
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::AfterMPCRun(std::stack<std::string> &token_stk,
                                           std::stack<TokenValue> &val_stk,
                                           std::string &new_token,
                                           TokenValue &res) {
  token_stk.push(new_token);
  token_val_map_[new_token] = res;
  val_stk.push(res);
}

template <Decimal Dbit> int MPCExpressExecutor<Dbit>::runMPCEvaluate(void) {
  std::stack<std::string> stk1;
  std::stack<TokenValue> val_stk;

  while (!suffix_stk_.empty()) {
    std::string token = suffix_stk_.top();
    if (token == "+") {
      std::string a, b;
      TokenValue val1, val2, res;
      BeforeMPCRun(stk1, val_stk, val1, val2, a, b);

      if (fp64_run_) {
        LOG(INFO) << "Run FP64 Add between '" << a << "' and '" << b << "'.";
        runMPCAddFP64(val1, val2, res);
        LOG(INFO) << "Run FP64 Add finish.";
      } else {
        LOG(INFO) << "Run I64 Add between '" << a << " and '" << b << "'.";
        si64Matrix sh_sum;
        runMPCAddI64(val1, val2, res);
        LOG(INFO) << "Run I64 Add finish.";
      }

      std::string new_token = "(" + a + "+" + b + ")";
      AfterMPCRun(stk1, val_stk, new_token, res);
    } else if (token == "-") {
      std::string a, b;
      TokenValue val1, val2, res;
      BeforeMPCRun(stk1, val_stk, val1, val2, a, b);

      if (fp64_run_) {
        LOG(INFO) << "Run FP64 Sub between '" << a << "' and '" << b << "'.";
        runMPCSubFP64(val1, val2, res);
        LOG(INFO) << "Run FP64 Sub finish.";
      } else {
        LOG(INFO) << "Run I64 Sub between '" << a << " and '" << b << "'.";
        si64Matrix sh_sum;
        runMPCSubI64(val1, val2, res);
        LOG(INFO) << "Run I64 Sub finish.";
      }

      std::string new_token = "(" + a + "-" + b + ")";
      AfterMPCRun(stk1, val_stk, new_token, res);
    } else if (token == "*") {
      std::string a, b;
      TokenValue val1, val2, res;
      BeforeMPCRun(stk1, val_stk, val1, val2, a, b);

      if (fp64_run_) {
        LOG(INFO) << "Run FP64 Mul between '" << a << "' and '" << b << "'.";
        runMPCMulFP64(val1, val2, res);
        LOG(INFO) << "Run FP64 Mul finish.";
      } else {
        LOG(INFO) << "Run I64 Mul between '" << a << " and '" << b << "'.";
        si64Matrix sh_sum;
        runMPCMulI64(val1, val2, res);
        LOG(INFO) << "Run I64 Mul finish.";
      }

      std::string new_token = "(" + a + "*" + b + ")";
      AfterMPCRun(stk1, val_stk, new_token, res);
    } else if (token == "/") {
      std::string a, b;
      TokenValue val1, val2, res;
      BeforeMPCRun(stk1, val_stk, val1, val2, a, b);
      LOG(INFO) << "Run FP64 Div between '" << a << "' and '" << b << "'.";
      runMPCDivFP64(val1, val2, res);
      LOG(INFO) << "Run FP64 Div finish.";
      std::string new_token = "(" + a + "/" + b + ")";
      AfterMPCRun(stk1, val_stk, new_token, res);
    } else {
      stk1.push(token);
      suffix_stk_.pop();

      TokenValue token_val;
      if (createTokenValue(token, token_val)) {
        LOG(ERROR) << "Construct token value for token '" << token
                   << "' failed.";
        return -1;
      }

      val_stk.push(token_val);
      token_val_map_[token] = token_val;
    }
  }

  while (!stk1.empty()) {
    suffix_stk_.push(stk1.top());
    stk1.pop();
  }

  return 0;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::revealMPCResult(std::vector<uint32_t> &parties,
                                               std::vector<double> &val_vec) {
  std::string final_token = suffix_stk_.top();
  suffix_stk_.pop();

  TokenValue val = token_val_map_[final_token];
  sf64Matrix<Dbit> *p_final_share = val.val_union.sh_fp64_m;

  for (auto party : parties) {
    if (party_id_ == party) {
      eMatrix<double> m = mpc_op_->reveal(*p_final_share);
      uint32_t count = 0;
      for (i64 i = 0; i < m.rows(); i++) {
        val_vec.emplace_back(m(i, 0));
        count++;
      }

      LOG(INFO) << "Reveal MPC result to party "
                << static_cast<char>(party + '0') << ", value count " << count
                << ".";
    } else {
      mpc_op_->reveal(*p_final_share, party);
    }
  }

  return;
}

template <Decimal Dbit>
void MPCExpressExecutor<Dbit>::revealMPCResult(std::vector<uint32_t> &parties,
                                               std::vector<int64_t> &val_vec) {
  std::string final_token = suffix_stk_.top();
  suffix_stk_.pop();

  TokenValue val = token_val_map_[final_token];
  si64Matrix *p_final_share = val.val_union.sh_i64_m;

  for (auto party : parties) {
    if (party_id_ == party) {
      i64Matrix m = mpc_op_->reveal(*p_final_share);
      uint32_t count = 0;
      for (i64 i = 0; i < m.rows(); i++) {
        val_vec.emplace_back(m(i, 0));
        count++;
      }

      LOG(INFO) << "Reveal MPC result to party "
                << static_cast<char>(party + '0') << ", value count " << count
                << ".";
    } else {
      mpc_op_->reveal(*p_final_share, party);
    }
  }

  return;
}

template <Decimal Dbit> void MPCExpressExecutor<Dbit>::Clean(void) {
  for (auto iter = token_val_map_.begin(); iter != token_val_map_.end();
       iter++) {
    switch (iter->second.type) {
    case 5:
      delete iter->second.val_union.sh_fp64_m;
      break;
    case 6:
      delete iter->second.val_union.sh_i64_m;
      break;
    default:
      break;
    }
  }

  if (col_config_) {
    delete col_config_;
    col_config_ = nullptr;
  }

  if (feed_dict_) {
    delete feed_dict_;
    feed_dict_ = nullptr;
  }

  while (!suffix_stk_.empty())
    suffix_stk_.pop();

  token_val_map_.clear();
  token_type_map_.clear();
}

template <Decimal Dbit> MPCExpressExecutor<Dbit>::~MPCExpressExecutor() {
  mpc_op_->fini();
  Clean();
}

template <Decimal Dbit>
void LocalExpressExecutor<Dbit>::beforeLocalCalculate(
    std::stack<std::string> &token_stk, std::stack<TokenValue> &val_stk,
    TokenValue &val1, TokenValue &val2, std::string &a, std::string &b) {
  mpc_exec_->BeforeMPCRun(token_stk, val_stk, val1, val2, a, b);
}

template <Decimal Dbit>
void LocalExpressExecutor<Dbit>::afterLocalCalculate(
    std::stack<std::string> &token_stk, std::stack<TokenValue> &val_stk,
    std::string &new_token, TokenValue &res) {
  token_stk.push(new_token);
  token_val_map_[new_token] = res;
  val_stk.push(res);
}

template <Decimal Dbit> int LocalExpressExecutor<Dbit>::runLocalEvaluate() {
  std::string expr = mpc_exec_->expr_;
  LOG(INFO) << expr;
  mpc_exec_->parseExpress(expr);

  std::stack<std::string> stk1;
  std::stack<TokenValue> val_stk;

  std::stack<std::string> &suffix_stk = mpc_exec_->suffix_stk_;
  while (!suffix_stk.empty()) {
    std::string token = suffix_stk.top();
    if (token == "+") {
      std::string a, b;
      TokenValue val1, val2, res;
      std::string new_token;
      beforeLocalCalculate(stk1, val_stk, val1, val2, a, b);
      if (mpc_exec_->fp64_run_) {
        std::vector<double> *p_vec_sum = new std::vector<double>();
        if (val1.type != 2 && val2.type != 2) {
          p_vec_sum->resize(val1.val_union.fp64_vec->size());
          for (size_t i = 0; i < p_vec_sum->size(); i++)
            (*p_vec_sum)[i] =
                (*val1.val_union.fp64_vec)[i] + (*val2.val_union.fp64_vec)[i];
        } else {
          if (val1.type == 2) {
            p_vec_sum->resize(val2.val_union.fp64_vec->size());
            for (size_t i = 0; i < p_vec_sum->size(); i++)
              (*p_vec_sum)[i] =
                  val1.val_union.fp64_val + (*val2.val_union.fp64_vec)[i];
          } else {
            p_vec_sum->resize(val1.val_union.fp64_vec->size());
            for (size_t i = 0; i < p_vec_sum->size(); i++)
              (*p_vec_sum)[i] =
                  val2.val_union.fp64_val + (*val1.val_union.fp64_vec)[i];
          }
        }
        res.val_union.fp64_vec = p_vec_sum;
        res.type = 0;
      } else {
        std::vector<int64_t> *p_vec_sum = new std::vector<int64_t>();
        if (val1.type != 3 && val2.type != 3) {
          p_vec_sum->resize(val1.val_union.i64_vec->size());
          for (size_t i = 0; i < p_vec_sum->size(); i++)
            (*p_vec_sum)[i] =
                (*val1.val_union.i64_vec)[i] + (*val2.val_union.i64_vec)[i];
        } else {
          if (val1.type == 3) {
            p_vec_sum->resize(val2.val_union.i64_vec->size());
            for (size_t i = 0; i < p_vec_sum->size(); i++)
              (*p_vec_sum)[i] =
                  val1.val_union.i64_val + (*val2.val_union.i64_vec)[i];
          } else {
            p_vec_sum->resize(val1.val_union.i64_vec->size());
            for (size_t i = 0; i < p_vec_sum->size(); i++)
              (*p_vec_sum)[i] =
                  val2.val_union.i64_val + (*val1.val_union.i64_vec)[i];
          }
        }
        res.val_union.i64_vec = p_vec_sum;
        res.type = 1;
      }
      new_token = "(" + a + "+" + b + ")";
      afterLocalCalculate(stk1, val_stk, new_token, res);
    } else if (token == "-") {
      std::string a, b;
      TokenValue val1, val2, res;
      std::string new_token;
      beforeLocalCalculate(stk1, val_stk, val1, val2, a, b);
      if (mpc_exec_->fp64_run_) {
        std::vector<double> *p_vec_sub = new std::vector<double>();
        if (val1.type != 2 && val2.type != 2) {
          p_vec_sub->resize(val1.val_union.fp64_vec->size());
          for (size_t i = 0; i < p_vec_sub->size(); i++)
            (*p_vec_sub)[i] =
                (*val1.val_union.fp64_vec)[i] - (*val2.val_union.fp64_vec)[i];
        } else {
          if (val1.type == 2) {
            p_vec_sub->resize(val2.val_union.fp64_vec->size());
            for (size_t i = 0; i < p_vec_sub->size(); i++)
              (*p_vec_sub)[i] =
                  val1.val_union.fp64_val - (*val2.val_union.fp64_vec)[i];
          } else {
            p_vec_sub->resize(val1.val_union.fp64_vec->size());
            for (size_t i = 0; i < p_vec_sub->size(); i++)
              (*p_vec_sub)[i] =
                  (*val1.val_union.fp64_vec)[i] - val2.val_union.fp64_val;
          }
        }
        res.val_union.fp64_vec = p_vec_sub;
        res.type = 0;
      } else {
        std::vector<int64_t> *p_vec_sub = new std::vector<int64_t>();
        if (val1.type != 3 && val2.type != 3) {
          p_vec_sub->resize(val1.val_union.i64_vec->size());
          for (size_t i = 0; i < p_vec_sub->size(); i++)
            (*p_vec_sub)[i] =
                (*val1.val_union.i64_vec)[i] - (*val2.val_union.i64_vec)[i];
        } else {
          if (val1.type == 3) {
            p_vec_sub->resize(val2.val_union.i64_vec->size());
            for (size_t i = 0; i < p_vec_sub->size(); i++)
              (*p_vec_sub)[i] =
                  val1.val_union.i64_val - (*val2.val_union.i64_vec)[i];
          } else {
            p_vec_sub->resize(val1.val_union.i64_vec->size());
            for (size_t i = 0; i < p_vec_sub->size(); i++)
              (*p_vec_sub)[i] =
                  (*val1.val_union.i64_vec)[i] - val2.val_union.i64_val;
          }
        }
        res.val_union.i64_vec = p_vec_sub;
        res.type = 1;
      }
      new_token = "(" + a + "-" + b + ")";
      afterLocalCalculate(stk1, val_stk, new_token, res);
    } else if (token == "*") {
      std::string a, b;
      TokenValue val1, val2, res;
      std::string new_token;
      beforeLocalCalculate(stk1, val_stk, val1, val2, a, b);
      if (mpc_exec_->fp64_run_) {
        std::vector<double> *p_vec_mul = new std::vector<double>();
        if (val1.type != 2 && val2.type != 2) {
          p_vec_mul->resize(val1.val_union.fp64_vec->size());
          for (size_t i = 0; i < p_vec_mul->size(); i++)
            (*p_vec_mul)[i] =
                (*val1.val_union.fp64_vec)[i] * (*val2.val_union.fp64_vec)[i];
        } else {
          if (val1.type == 2) {
            p_vec_mul->resize(val2.val_union.fp64_vec->size());
            for (size_t i = 0; i < p_vec_mul->size(); i++)
              (*p_vec_mul)[i] =
                  val1.val_union.fp64_val * (*val2.val_union.fp64_vec)[i];
          } else {
            p_vec_mul->resize(val1.val_union.fp64_vec->size());
            for (size_t i = 0; i < p_vec_mul->size(); i++)
              (*p_vec_mul)[i] =
                  (*val1.val_union.fp64_vec)[i] * val2.val_union.fp64_val;
          }
        }
        res.val_union.fp64_vec = p_vec_mul;
        res.type = 0;
      } else {
        std::vector<int64_t> *p_vec_mul = new std::vector<int64_t>();
        if (val1.type != 3 && val2.type != 3) {
          p_vec_mul->resize(val1.val_union.i64_vec->size());
          for (size_t i = 0; i < p_vec_mul->size(); i++)
            (*p_vec_mul)[i] =
                (*val1.val_union.i64_vec)[i] * (*val2.val_union.i64_vec)[i];
        } else {
          if (val1.type == 3) {
            p_vec_mul->resize(val2.val_union.i64_vec->size());
            for (size_t i = 0; i < p_vec_mul->size(); i++)
              (*p_vec_mul)[i] =
                  val1.val_union.i64_val * (*val2.val_union.i64_vec)[i];
          } else {
            p_vec_mul->resize(val1.val_union.i64_vec->size());
            for (size_t i = 0; i < p_vec_mul->size(); i++)
              (*p_vec_mul)[i] =
                  (*val1.val_union.i64_vec)[i] * val2.val_union.i64_val;
          }
        }
        res.val_union.i64_vec = p_vec_mul;
        res.type = 1;
      }
      new_token = "(" + a + "*" + b + ")";
      afterLocalCalculate(stk1, val_stk, new_token, res);
    } else if (token == "/") {
      std::string a, b;
      TokenValue val1, val2, res;
      std::string new_token;
      beforeLocalCalculate(stk1, val_stk, val1, val2, a, b);
      std::vector<double> *p_vec_div = new std::vector<double>();
      if (val1.type != 2 && val2.type != 2) {
        p_vec_div->resize(val1.val_union.fp64_vec->size());
        for (size_t i = 0; i < p_vec_div->size(); i++)
          (*p_vec_div)[i] =
              (*val1.val_union.fp64_vec)[i] / (*val2.val_union.fp64_vec)[i];
      } else {
        if (val1.type == 2) {
          p_vec_div->resize(val2.val_union.fp64_vec->size());
          for (size_t i = 0; i < p_vec_div->size(); i++)
            (*p_vec_div)[i] =
                val1.val_union.fp64_val / (*val2.val_union.fp64_vec)[i];
        } else {
          p_vec_div->resize(val1.val_union.fp64_vec->size());
          for (size_t i = 0; i < p_vec_div->size(); i++)
            (*p_vec_div)[i] =
                (*val1.val_union.fp64_vec)[i] / val2.val_union.fp64_val;
        }
      }
      res.val_union.fp64_vec = p_vec_div;
      res.type = 0;
      new_token = "(" + a + "/" + b + ")";
      afterLocalCalculate(stk1, val_stk, new_token, res);
    } else {
      stk1.push(token);
      suffix_stk.pop();
      TokenValue token_val;
      if (createTokenValue(token, token_val)) {
        LOG(ERROR) << "Construct token value for token '" << token
                   << "' failed.";
        return -1;
      }
      val_stk.push(token_val);
      token_val_map_[token] = token_val;
    }
  }
  while (!stk1.empty()) {
    suffix_stk.push(stk1.top());
    stk1.pop();
  }
  std::string final_token = suffix_stk.top();
  suffix_stk.pop();
  TokenValue finalVal = token_val_map_[final_token];
  if (finalVal.type == 0) {
    for (size_t i = 0; i < finalVal.val_union.fp64_vec->size(); i++) {
      final_val_double.push_back((*finalVal.val_union.fp64_vec)[i]);
    }
  } else {
    for (size_t i = 0; i < finalVal.val_union.i64_vec->size(); i++) {
      final_val_int64.push_back((*finalVal.val_union.i64_vec)[i]);
    }
  }
  // return
  return 0;
}

template <Decimal Dbit>
void LocalExpressExecutor<Dbit>::createNewColumnConfig() {
  new_col_cfg = new typename MPCExpressExecutor<Dbit>::ColumnConfig(
      mpc_exec_->col_config_->party_id_);
  std::map<std::string, bool> &local_col_outside =
      mpc_exec_->col_config_->local_col_;
  for (auto &pair : local_col_outside)
    new_col_cfg->local_col_.insert(std::make_pair(pair.first, true));
  std::map<std::string,
           typename MPCExpressExecutor<Dbit>::ColumnConfig::ColDtype>
      &local_col_dtype = mpc_exec_->col_config_->col_dtype_;
  for (auto &pair : local_col_dtype)
    new_col_cfg->col_dtype_.insert(std::make_pair(pair.first, pair.second));

  std::map<std::string,
           typename MPCExpressExecutor<Dbit>::ColumnConfig::ColDtype>
      col_dtype_;
}

template <Decimal Dbit> void LocalExpressExecutor<Dbit>::creatNewFeedDict() {
  new_feed = new typename MPCExpressExecutor<Dbit>::FeedDict(
      new_col_cfg, mpc_exec_->fp64_run_);
}

template <Decimal Dbit>
int LocalExpressExecutor<Dbit>::createTokenValue(const std::string &token,
                                                 TokenValue &token_val) {
  typename MPCExpressExecutor<Dbit>::TokenType type =
      mpc_exec_->token_type_map_[token];
  if (type == MPCExpressExecutor<Dbit>::TokenType::COLUMN) {
    if (mpc_exec_->fp64_run_) {
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
    if (mpc_exec_->fp64_run_) {
      token_val.val_union.fp64_val = std::stod(token);
      token_val.type = 2;
    } else {
      token_val.val_union.i64_val = atol(token.c_str());
      token_val.type = 3;
    }
  }
  return 0;
}

template <Decimal Dbit> LocalExpressExecutor<Dbit>::~LocalExpressExecutor() {
  for (auto iter = token_val_map_.begin(); iter != token_val_map_.end();
       iter++) {
    switch (iter->second.type) {
    case 5:
      delete iter->second.val_union.sh_fp64_m;
      break;
    case 6:
      delete iter->second.val_union.sh_i64_m;
      break;
    default:
      break;
    }
  }
  delete new_feed;
  delete new_col_cfg;
  token_val_map_.clear();
}
template class MPCExpressExecutor<D32>;
template class LocalExpressExecutor<D32>;

template class MPCExpressExecutor<D16>;
template class LocalExpressExecutor<D16>;
} // namespace primihub
