// "Copyright [2023] <PrimiHub>"
#ifndef SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_H_
#define SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_H_
#include <variant>
#include <vector>
#include <memory>
#include <utility>
#include <string>

#include "src/primihub/kernel/pir/operator/base_pir.h"
#include "src/primihub/kernel/pir/common.h"

// APSI
#include "apsi/thread_pool_mgr.h"
#include "apsi/sender_db.h"
#include "apsi/oprf/oprf_sender.h"
#include "apsi/powers.h"
#include "apsi/util/common_utils.h"
#include "apsi/sender.h"
#include "apsi/bin_bundle.h"
#include "apsi/item.h"
#include "apsi/receiver.h"

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

class KeywordPirOperator : public BasePirOperator {
 public:
  enum class RequestType : uint8_t {
    PsiParam = 0,
    Oprf,
    Query,
  };
  explicit KeywordPirOperator(const Options& options) :
      BasePirOperator(options) {}

  retcode OnExecute(const PirDataType& input, PirDataType* result) override;

 protected:
  retcode ExecuteAsClient(const PirDataType& input, PirDataType* result);
  retcode ExecuteAsServer(const PirDataType& input);


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

  std::shared_ptr<apsi::sender::SenderDB>
  LoadDbFromCache(const std::string& db_path);

 private:
  std::string psi_params_str_;
  std::unique_ptr<apsi::oprf::OPRFKey> oprf_key_{nullptr};
  std::unique_ptr<apsi::receiver::Receiver> receiver_{nullptr};
  std::unique_ptr<apsi::PSIParams> psi_params_{nullptr};
};
}  // namespace primihub::pir
#endif  // SRC_PRIMIHUB_KERNEL_PIR_OPERATOR_KEYWORD_PIR_H_
