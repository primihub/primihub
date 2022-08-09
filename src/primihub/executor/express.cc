#include <glog/logging.h>

#include "src/primihub/executor/express.h"

MPCExpressExecutor::MPCExpressExecutor() {}

bool MPCExpressExecutor::isOperator(const char op) {
  if (op == '+' || op == '-' || op == '*' || op == '/')
    return true;
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
  std::stack<std::string> tmp_stk = suffix_stk;

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
