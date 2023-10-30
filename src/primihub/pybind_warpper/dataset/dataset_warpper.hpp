/*
 Copyright 2022 PrimiHub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */


#include <iostream>
#include <memory>

#include <pybind11/pybind11.h>
#include <arrow/array.h>
#include <arrow/python/pyarrow.h>
#include <arrow/table.h>

#include "src/primihub/data_store/dataset.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/data_store/factory.h"


using namespace arrow;

using primihub::Dataset;
using primihub::DataDriver;
using primihub::DataDirverFactory;
using primihub::service::DatasetService;
using primihub::service::DatasetMeta;
using primihub::service::DatasetVisbility;


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
}  // namespace pybind11::detail


// NOTE !!! only for test !!!
void test_unwrap_arrow_pyobject(std::shared_ptr<arrow::Table> &table) {
    std::cout << "------- unwrap_arrow_pyobject-----" << std::endl;
    std::cout << "Table schema: " << std::endl;
    std::cout << table->schema()->ToString() << std::endl;
}

// Register apache arrow table object as primihub dataset and publish on DHT.
// NOTE default using csv driver
void reg_arrow_table_as_ph_dataset(std::shared_ptr<DatasetService> &dataset_service,
                                   std::string &dataset_name,
                                   std::shared_ptr<arrow::Table> &table) {
    // Construct primihub::dataset object from arrow table.
    std::shared_ptr<primihub::DataDriver> driver =
        primihub::DataDirverFactory::getDriver("CSV", dataset_service->getNodeletAddr());
    std::string filepath = "data/" + dataset_name + ".csv";
    auto cursor = driver->initCursor(filepath);
    auto dataset = std::make_shared<primihub::Dataset>(table, driver);

    // Get dataset meta form dataset object.
    DatasetMeta meta(dataset, dataset_name, DatasetVisbility::PUBLIC);

    // Register meta on DHT.
    dataset_service->regDataset(meta);
}
