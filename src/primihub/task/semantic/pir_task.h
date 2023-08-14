//  Copyright 2023 <PrimiHub>
#ifndef SRC_PRIMIHUB_TASK_SEMANTIC_PIR_TASK_H_
#define SRC_PRIMIHUB_TASK_SEMANTIC_PIR_TASK_H_
#include <string>
#include "src/primihub/task/semantic/task.h"
#include "src/primihub/common/common.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/kernel/pir/common.h"
#include "src/primihub/kernel/pir/operator/base_pir.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
namespace primihub::task {
using BasePirOperator = primihub::pir::BasePirOperator;

class PirTask : public TaskBase {
 public:
  PirTask(const TaskParam *task_param,
          std::shared_ptr<DatasetService> dataset_service);
  ~PirTask() = default;
  int execute() override;

 protected:
  retcode LoadParams(const rpc::Task& task);
  retcode GetServerDataSetSchema(const rpc::Task& task);
  retcode LoadDataset();
  retcode ClientLoadDataset();
  retcode ServerLoadDataset();
  std::shared_ptr<Dataset> LoadDataSetInternal(const std::string& dataset_id);
  bool DbCacheAvailable(const std::string& db_file_cache) {
    return FileExists(db_file_cache);
  }
  std::vector<std::string> GetSelectedContent(
      std::shared_ptr<arrow::Table>& data_tbl,
      const std::vector<int>& selected_col);
  retcode SaveResult();
  retcode InitOperator();
  retcode ExecuteOperator();
  retcode BuildOptions(const rpc::Task& task,
                       primihub::pir::Options* option);
  bool NeedSaveResult();


 private:
  int pir_type_{rpc::PirType::KEY_PIR};
  std::string dataset_path_;
  std::string dataset_id_;
  std::string result_file_path_;
  primihub::pir::PirDataType elements_;
  primihub::pir::PirDataType result_;
  primihub::pir::Options options_;
  std::string db_cache_dir_{"data/cache"};
  std::unique_ptr<BasePirOperator> operator_{nullptr};
  std::vector<std::string> server_dataset_schema_;

  // std::string dataset_path_;
  // std::string dataset_id_;

  // std::string db_file_cache_;
  // primihub::Node client_node_;
  // std::string key{"key_pir"};
  // std::string psi_params_str_;
  // std::unique_ptr<apsi::oprf::OPRFKey> oprf_key_{nullptr};
  // bool generate_db_offline_{false};
};
}  // namespace primihub::task
#endif  // SRC_PRIMIHUB_TASK_SEMANTIC_PIR_TASK_H_
