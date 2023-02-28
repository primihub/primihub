#include "gtest/gtest.h"

#include "src/primihub/task/language/py_parser.h"


namespace primihub::task {
TEST(PyParserTest, PyParserTest_parserDatasets) {
    const char* py_code_c =
        "from primihub import dataset \n"
        "dataset.get('test_data') \n"
        "dataset.get('test_label')";

    // std::string py_code(py_code_c);
    // auto ds = PyParser(py_code).parseDatasets();
    // ASSERT_EQ(ds.size(), 2);
}

}  // namespace primihub::task
