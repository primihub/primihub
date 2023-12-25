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

#ifndef SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_IMPL_KEYWORD_PIR_SERVER_H_
#define SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_IMPL_KEYWORD_PIR_SERVER_H_

#include <variant>
#include <vector>
#include <memory>
#include <utility>
#include <string>

#include "src/primihub/kernel/pir/operator/base_pir.h"
#include "src/primihub/kernel/pir/common.h"
#include "src/primihub/kernel/pir/operator/keyword_pir_impl/keyword_pir_common.h"

// APSI
#include "apsi/thread_pool_mgr.h"
#include "apsi/sender_db.h"
#include "apsi/oprf/oprf_sender.h"
#include "apsi/powers.h"
#include "apsi/util/common_utils.h"
#include "apsi/sender.h"
#include "apsi/bin_bundle.h"
#include "apsi/item.h"

// SEAL
#include "seal/context.h"
#include "seal/modulus.h"
#include "seal/util/common.h"
#include "seal/util/defines.h"

using namespace apsi;           // NOLINT
using namespace apsi::sender;   // NOLINT
using namespace apsi::oprf;     // NOLINT
using namespace apsi::network;  // NOLINT

using namespace seal;           // NOLINT
using namespace seal::util;     // NOLINT

namespace primihub::pir {
using UnlabeledData = std::vector<apsi::Item>;
using LabeledData = std::vector<std::pair<apsi::Item, apsi::Label>>;
using DBData = std::variant<UnlabeledData, LabeledData>;

class KeywordPirOperatorServer : public BasePirOperator {
 public:
  explicit KeywordPirOperatorServer(const Options& options) :
      BasePirOperator(options) {}

  retcode OnExecute(const PirDataType& input, PirDataType* result) override;

 protected:
  // ------------------------Sender----------------------------
  std::unique_ptr<apsi::PSIParams> SetPsiParams();
  /**
   * process a Get Parameters request to the Sender.
  */
  retcode ProcessPSIParams();
  /**
    process an OPRF query request to the Sender.
  */
  retcode ProcessOprf();
  /**
    process a Query request to the Sender.
  */
  retcode ProcessQuery(std::shared_ptr<apsi::sender::SenderDB> sender_db);

  retcode ComputePowers(const shared_ptr<apsi::sender::SenderDB> &sender_db,
                        const apsi::CryptoContext &crypto_context,
                        std::vector<apsi::sender::CiphertextPowers> &all_powers,
                        const apsi::PowersDag &pd,
                        uint32_t bundle_idx,
                        seal::MemoryPoolHandle &pool);

  auto ProcessBinBundleCache(
      const shared_ptr<apsi::sender::SenderDB> &sender_db,
      const apsi::CryptoContext &crypto_context,
      reference_wrapper<const apsi::sender::BinBundleCache> cache,
      std::vector<apsi::sender::CiphertextPowers> &all_powers,
      uint32_t bundle_idx,
      compr_mode_type compr_mode,
      seal::MemoryPoolHandle &pool) ->
      std::unique_ptr<apsi::network::ResultPackage>;

  std::unique_ptr<DBData> CreateDb(const PirDataType& input);
  retcode CreateDbDataCache(const DBData& db_data,
                            std::unique_ptr<apsi::PSIParams> psi_params,
                            apsi::oprf::OPRFKey &oprf_key,
                            size_t nonce_byte_count,
                            bool compress);

  auto CreateSenderDb(const DBData &db_data,
                      std::unique_ptr<PSIParams> psi_params,
                      apsi::oprf::OPRFKey &oprf_key,
                      size_t nonce_byte_count,
                      bool compress) -> std::shared_ptr<SenderDB>;
  bool DbCacheAvailable(const std::string& db_path);

  auto LoadDbFromCache(const std::string& db_path) ->
      std::shared_ptr<apsi::sender::SenderDB>;

 private:
  std::string psi_params_str_;
  std::unique_ptr<apsi::oprf::OPRFKey> oprf_key_{nullptr};
  std::unique_ptr<apsi::PSIParams> psi_params_{nullptr};
};
}  // namespace primihub::pir
#endif  // SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_IMPL_KEYWORD_PIR_SERVER_H_
