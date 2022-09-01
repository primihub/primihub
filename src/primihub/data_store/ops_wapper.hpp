
#include <iostream>

#include <pybind11/pybind11.h>
#include <arrow/array.h>
#include <arrow/python/pyarrow.h>
#include <arrow/table.h>

// #include "src/primihub/data_store/arrow_conversions.h"
// #include "src/primihub/data_store/arrow_conversions.h"
using namespace arrow;

namespace pybind11
{
    namespace detail
    {
        template <typename TableType>
        struct gen_type_caster
        {
        public:
            PYBIND11_TYPE_CASTER(std::shared_ptr<TableType>, _("pyarrow::Table"));
            // Python -> C++
            bool load(handle src, bool)
            {
                PyObject *source = src.ptr();
                if (!arrow::py::is_table(source))
                    return false;
                arrow::Result<std::shared_ptr<arrow::Table>> result = arrow::py::unwrap_table(source);
                if (!result.ok())
                    return false;
                value = std::static_pointer_cast<TableType>(result.ValueOrDie());
                return true;
            }
            // C++ -> Python
            static handle cast(std::shared_ptr<TableType> src, return_value_policy /* policy */, handle /* parent */)
            {
                return arrow::py::wrap_table(src);
            }
        };
        template <>
        struct type_caster<std::shared_ptr<arrow::Table>> : public gen_type_caster<arrow::Table>
        {
        };
    }
} // namespace pybind11::detail


//!!! only for test !!!
void unwrap_arrow_pyobject(std::shared_ptr<arrow::Table> &table) {
    std::cout << "------- unwrap_arrow_pyobject-----" << std::endl;
    std::cout << "Table schema: " << std::endl;
    std::cout << table->schema()->ToString() << std::endl;
}
