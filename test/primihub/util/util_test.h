using namespace primihub;

class UnitTestFail : public std::exception {
  std::string mWhat;
 public:
  explicit UnitTestFail(std::string reason)
    :std::exception(),
    mWhat(reason)
  {}

  explicit UnitTestFail() :std::exception(),
    mWhat("UnitTestFailed exception") { }

  virtual const char* what() const throw() {
    return mWhat.c_str();
  }
};

class UnitTestSkipped : public std::runtime_error {
 public:
  UnitTestSkipped()
      : std::runtime_error("skipping test")
  {}

  UnitTestSkipped(std::string r) : std::runtime_error(r)
  {}
};
