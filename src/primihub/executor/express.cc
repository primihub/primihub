#include <glog/logging.h>

#include "src/primihub/executor/express.h"

using TokenValue = primihub::MPCExpressExecutor::TokenValue;

namespace primihub {
// Implement of class ColumnConf.
std::string ColumnConfig::DtypeToString(const ColDtype &dtype) {
  if (dtype == ColDtype::FP64)
    return std::string("FP64");
  else
    return std::string("INT64");
}

int ColumnConfig::importColumnDtype(const std::string &col_name,
                                    const ColDtype &dtype) {
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

int ColumnConfig::importColumnOwner(const std::string &col_name,
                                    const std::string &node_id) {
  auto iter = col_owner_.find(col_name);
  if (iter != col_owner_.end()) {
    LOG(ERROR) << "Column " << col_name
               << "'s owner attr is imported, value is " << iter->second << ".";
    return -1;
  }

  col_owner_.insert(std::make_pair(col_name, node_id));
  return 0;
}

int ColumnConfig::getColumnDtype(const std::string &col_name, ColDtype &dtype) {
  auto iter = col_dtype_.find(col_name);
  if (iter == col_dtype_.end()) {
    LOG(ERROR) << "Can't find dtype attr for column " << col_name << ".";
    return -1;
  }

  dtype = iter->second;
  return 0;
}

int ColumnConfig::resolveLocalColumn(void) {
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
    if (iter->second == node_id_)
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
    LOG(INFO) << "Dump finish, dump count " << count << ".";
  }

  return 0;
}

int ColumnConfig::getColumnLocality(const std::string &col_name,
                                    bool &is_local) {
  auto iter = local_col_.find(col_name);
  if (iter == local_col_.end()) {
    LOG(ERROR) << "Can't find column locality by column name " << col_name
               << ".";
    return -1;
  }

  is_local = iter->second;
  return 0;
}

bool ColumnConfig::hasFP64Column(void) {
  bool fp64_col = false;
  for (auto iter = col_dtype_.begin(); iter != col_dtype_.end(); iter++) {
    if (iter->second == ColDtype::FP64) {
      fp64_col = true;
      break;
    }
  }

  return fp64_col;
}

void ColumnConfig::Clean(void) {
  col_owner_.clear();
  col_dtype_.clear();
  local_col_.clear();
  node_id_ = "";
}

ColumnConfig::~ColumnConfig() { Clean(); }

// Implement of class FeedDict.
int FeedDict::checkLocalColumn(const std::string &col_name) {
  bool is_local = false;
  int ret = col_config_->getColumnLocality(col_name, is_local);
  if (ret) {
    LOG(ERROR) << "Get column locality by column name " << col_name
               << " failed.";
    return -1;
  }

  if (is_local == false)
    return 0;
}

int FeedDict::importColumnValues(const std::string &col_name,
                                 std::vector<int64_t> &int64_vec) {
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

  ColumnConfig::ColDtype dtype;
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

int FeedDict::importColumnValues(const std::string &col_name,
                                 std::vector<double> &fp64_vec) {
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

int FeedDict::getColumnValues(const std::string &col_name,
                              std::vector<int64_t> **p_col_vec) {
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

int FeedDict::getColumnValues(const std::string &col_name,
                              std::vector<double> **p_col_vec) {
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

FeedDict::~FeedDict() {
  this->float_run_ = false;
  this->fp64_col_.clear();
  this->int64_col_.clear();
  this->col_config_ = nullptr;
}

// Implement of class MPCExpressExecutor.
MPCExpressExecutor::MPCExpressExecutor() {}

bool MPCExpressExecutor::isOperator(const char op) {
  if (op == '+' || op == '-' || op == '*' || op == '/')
    return true;
  return false;
}

bool MPCExpressExecutor::isOperator(const std::string &op) {
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

int MPCExpressExecutor::Priority(const char str) {
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

bool MPCExpressExecutor::checkExpress(void) {
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

  if ((suffix_vec.size() - num_op) != (num_op + 1)) {
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

void MPCExpressExecutor::parseExpress(const std::string &expr) {
  std::string temp;
  std::string op;
  std::string suffix;
  std::stack<std::string> stk2;

  for (int i = 0; i <= expr.length(); i++) {
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
      // str[i] is a charactor of column name or a constant.
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

int MPCExpressExecutor::importExpress(std::string expr) {
  parseExpress(expr);
  bool ret = checkExpress();
  if (ret == false) {
    LOG(ERROR) << "Import express '" << expr << "' failed.";
    return -1;
  }

  return 0;
}

void MPCExpressExecutor::resolveRunMode(void) {
  std::stack<std::string> tmp_suffix = suffix_stk_;
  bool has_div_op = false;
  bool has_fp64_val = false;

  while (!tmp_suffix.empty()) {
    std::string token = tmp_suffix.top();
    tmp_suffix.pop();

    if (isOperator(token)) {
      // Token is an operator.
      if (token == "/")
        has_div_op = true;
      continue;
    }

    ColumnConfig::ColDtype dtype;
    if (!col_config_->getColumnDtype(token, dtype)) {
      // Token is a column name.
      token_type_.insert(std::make_pair(token, TokenType::COLUMN));
      continue;
    }

    // Token is a number.
    token_type_.insert(std::make_pair(token, TokenType::VALUE));
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
    LOG(INFO) << "MPC run in INT64 mode.";
}

void MPCExpressExecutor::initMPCRuntime(uint8_t party_id, const std::string &ip,
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

  mpc_op_ = new MPCOperator(party_id, next_name, prev_name);
  mpc_op_->setup(ip, next_port, prev_port);

  return;
}

void MPCExpressExecutor::constructI64Matrix(TokenValue &val, i64Matrix &m) {
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

void MPCExpressExecutor::createI64Shares(TokenValue &val1, TokenValue &val2,
                                         si64Matrix &sh_val1,
                                         si64Matrix &sh_val2) {
  if (val1.type == 4) {
    mpc_op_->createShares(sh_val1);
  } else {
    i64Matrix m;
    constructI64Matrix(val1, m);
    mpc_op_->createShares(m, sh_val1);
  }

  if (val2.type == 4) {
    mpc_op_->createShares(sh_val2);
  } else {
    i64Matrix m;
    constructI64Matrix(val2, m);
    mpc_op_->createShares(m, sh_val2);
  }

  return;
}

void MPCExpressExecutor::constructFP64Matrix(TokenValue &val,
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

void MPCExpressExecutor::createFP64Shares(TokenValue &val1, TokenValue &val2,
                                          sf64Matrix<D> &sh_val1,
                                          sf64Matrix<D> &sh_val2) {
  if (val1.type == 4) {
    mpc_op_->createShares<D>(sh_val1);
  } else {
    eMatrix<double> m;
    constructFP64Matrix(val1, m);
    mpc_op_->createShares<D>(m, sh_val1);
  }

  if (val2.type == 4) {
    mpc_op_->createShares<D>(sh_val2);
  } else {
    eMatrix<double> m;
    constructFP64Matrix(val1, m);
    mpc_op_->createShares<D>(m, sh_val2);
  }

  return;
}

void MPCExpressExecutor::createTokenValue(const std::string &token,
                                          TokenValue &token_val) {
  TokenType type = token_type_[token];
  if (type == TokenType::COLUMN) {
    // Token is a column name, a remote column or a local column.
    bool is_local = false;
    col_config_->getColumnLocality(token, is_local);
    if (is_local == false) {
      // Column is a remote column.
      token_val.type = 4;
      VLOG(3) << "Construct TokenValue instance for '" << token << "', type '"
              << token_val.TypeToString() << "'.";
      return;
    }

    // Column is a local column.
    if (fp64_run_) {
      feed_dict_->getColumnValues(token, &(token_val.val_union.fp64_vec));
      token_val.type = 0;
    } else {
      feed_dict_->getColumnValues(token, &(token_val.val_union.i64_vec));
      token_val.type = 1;
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

  VLOG(3) << "Construct TokenValue instance for '" << token << "', type '"
          << token_val.TypeToString() << "'.";
}

void MPCExpressExecutor::runMPCAdd(const std::string &token1,
                                   const std::string &token2,
                                   sf64Matrix<D> &sh_res) {
  TokenValue val1, val2;
  createTokenValue(token1, val1);
  createTokenValue(token2, val2);

  sf64Matrix<D> sh_val1, sh_val2;
  createFP64Shares(val1, val2, sh_val1, sh_val2);

  std::vector<sf64Matrix<D>> sh_val_vec;
  sh_val_vec.emplace_back(sh_val1);
  sh_val_vec.emplace_back(sh_val2);

  sh_res = mpc_op_->MPC_Add(sh_val_vec);
}

void MPCExpressExecutor::runMPCAdd(const std::string &token1,
                                   const std::string &token2,
                                   si64Matrix &sh_res) {
  TokenValue val1, val2;
  createTokenValue(token1, val1);
  createTokenValue(token2, val2);

  si64Matrix sh_val1, sh_val2;
  createI64Shares(val1, val2, sh_val1, sh_val2);

  std::vector<si64Matrix> sh_val_vec;
  sh_val_vec.emplace_back(sh_val1);
  sh_val_vec.emplace_back(sh_val2);

  sh_res = mpc_op_->MPC_Add(sh_val_vec);
}

int MPCExpressExecutor::runMPCEvaluate(void) {
  std::stack<std::string> stk1;

  while (!suffix_stk_.empty()) {
    std::string token = suffix_stk_.top();
    if (token == "+") {
      std::string a = stk1.top();
      stk1.pop();
      std::string b = stk1.top();
      stk1.pop();
      suffix_stk_.pop();

      if (fp64_run_) {
        VLOG(3) << "Run FP64 add between '" << a << ", and '" << b << "'.";
        sf64Matrix<D> sh_sum;
        runMPCAdd(a, b, sh_sum);
        VLOG(3) << "Finish.";
      } else {
        VLOG(3) << "Run I64 add between '" << a << ", and '" << b << "'.";
        si64Matrix sh_sum;
        runMPCAdd(a, b, sh_sum);
        VLOG(3) << "Finish.";
      }
    } else if (token == "-") {
      std::string a = stk1.top();
      stk1.pop();
      std::string b = stk1.top();
      stk1.pop();
      suffix_stk_.pop();

    } else if (token == "*") {
      std::string a = stk1.top();
      stk1.pop();
      std::string b = stk1.top();
      stk1.pop();
      suffix_stk_.pop();

    } else if (token == "/") {
      std::string a = stk1.top();
      stk1.pop();
      std::string b = stk1.top();
      stk1.pop();
      suffix_stk_.pop();

    } else {
      stk1.push(token);
      suffix_stk_.pop();
    }
  }
}

MPCExpressExecutor::~MPCExpressExecutor() {
  token_type_.clear();

  while (!suffix_stk_.empty())
    suffix_stk_.pop();

  delete mpc_op_;
}

} // namespace primihub
