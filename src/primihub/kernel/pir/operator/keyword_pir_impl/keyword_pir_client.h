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

#ifndef SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_IMPL_KEYWORD_PIR_CLIENT_H_
#define SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_IMPL_KEYWORD_PIR_CLIENT_H_

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
#include "apsi/oprf/oprf_sender.h"
#include "apsi/powers.h"
#include "apsi/util/common_utils.h"
#include "apsi/bin_bundle.h"
#include "apsi/item.h"
#include "apsi/receiver.h"

// SEAL
#include "seal/context.h"
#include "seal/modulus.h"
#include "seal/util/common.h"
#include "seal/util/defines.h"

using namespace apsi;           // NOLINT
using namespace apsi::oprf;     // NOLINT
using namespace apsi::network;  // NOLINT

using namespace seal;           // NOLINT
using namespace seal::util;     // NOLINT

namespace primihub::pir {
using UnlabeledData = std::vector<apsi::Item>;

class KeywordPirOperatorClient : public BasePirOperator {
 public:
  explicit KeywordPirOperatorClient(const Options& options) :
      BasePirOperator(options) {}

  retcode OnExecute(const PirDataType& input, PirDataType* result) override;


 protected:
  // ------------------------Receiver----------------------------
  /**
  * Performs a parameter request from sender
  */
  retcode RequestPSIParams();
  /**
  * Performs an OPRF request on a vector of items and returns a
    vector of OPRF hashed items of the same size as the input vector.
  */
  retcode RequestOprf(const std::vector<Item>& items,
        std::vector<apsi::HashedItem>*, std::vector<apsi::LabelKey>*);
  /**
  * Performs a labeled PSI query. The query is a vector of items,
  * and the result is a same-size vector of MatchRecord objects.
  * If an item is in the intersection,
  * the corresponding MatchRecord indicates it in the `found` field,
  * and the `label` field may contain the corresponding
  * label if a sender's data included it.
  */
  retcode RequestQuery();
  retcode ExtractResult(const std::vector<std::string>& orig_vec,
      const std::vector<apsi::receiver::MatchRecord>& query_result,
      PirDataType* result);

 private:
  std::unique_ptr<apsi::receiver::Receiver> receiver_{nullptr};
  std::unique_ptr<apsi::PSIParams> psi_params_{nullptr};
};
}  // namespace primihub::pir
#endif  // SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_IMPL_KEYWORD_PIR_CLIENT_H_
