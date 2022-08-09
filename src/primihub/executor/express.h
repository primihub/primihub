#ifndef SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_
#define SRC_PRIMIHUB_EXECUTOR_EXPRESS_H_

#include <cstdlib>
#include <iostream>
#include <string>
#include <stack>

namespace primihub {
class MPCExpressExecutor {
public:
  MPCExpressExecutor();
  ~MPCExpressExecutor();
  int ImportExpress(std::string expr);

private:
  bool isOperator(const char op);
  int Priority(const char str);
  bool checkExpress(void);
  void parseExpress(const std::string &expr);

  std::string expr_;
  std::stack<std::string> suffix_stk_;
};
}; // namespace primihub
#endif
