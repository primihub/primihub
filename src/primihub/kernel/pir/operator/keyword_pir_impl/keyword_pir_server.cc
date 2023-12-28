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

#include "src/primihub/kernel/pir/operator/keyword_pir_impl/keyword_pir_server.h"
#include <fstream>
#include <algorithm>
#include <unordered_map>

#include "src/primihub/common/value_check_util.h"
#include "src/primihub/util/util.h"
#include "src/primihub/util/file_util.h"
#include "src/primihub/common/common.h"

namespace primihub::pir {
using Sender = apsi::sender::Sender;
using OPRFKey = apsi::oprf::OPRFKey;
using SenderDB = apsi::sender::SenderDB;
using CryptoContext = apsi::CryptoContext;
using SenderOperationQuery = apsi::network::SenderOperationQuery;
using CiphertextPowers = apsi::sender::CiphertextPowers;
using PowersDag = apsi::PowersDag;
using PSIParams = apsi::PSIParams;

retcode KeywordPirOperatorServer::OnExecute(const PirDataType& input,
                                            PirDataType* result) {
  VLOG(0) << "enter KeywordPirOperator  OPRFKey ctr";
  this->oprf_key_ = std::make_unique<apsi::oprf::OPRFKey>();
  VLOG(0) << "exit enter KeywordPirOperator OPRFKey ctr";
  size_t cpu_core_num = std::thread::hardware_concurrency();
  size_t use_core_num = cpu_core_num / 2;
  LOG(INFO) << "ThreadPoolMgr thread count: " << use_core_num;
  ThreadPoolMgr::SetThreadCount(use_core_num);

  auto params = SetPsiParams();
  CHECK_NULLPOINTER(params, retcode::FAIL);
  if (this->options_.generate_db) {
    // generate db offline which can load when task execute
    auto db_data = CreateDb(input);
    CHECK_NULLPOINTER(db_data, retcode::FAIL);
    auto ret = CreateDbDataCache(*db_data, std::move(params),
                                 *(this->oprf_key_), 16, false);
    if (ret != retcode::SUCCESS) {
      LOG(ERROR) << "CreateDbDataCache failed.";
      return retcode::FAIL;
    }
    return retcode::SUCCESS;
  }

  auto ret = ProcessPSIParams();
  CHECK_RETCODE_WITH_RETVALUE(ret, retcode::FAIL);
  std::shared_ptr<SenderDB> sender_db{nullptr};
  if (DbCacheAvailable(this->options_.db_path)) {
    sender_db = LoadDbFromCache(this->options_.db_path);
  } else {
    // std::unique_ptr<DBData>
    auto db_data = CreateDb(input);
    CHECK_NULLPOINTER(db_data, retcode::FAIL);
    // OPRFKey oprf_key;
    sender_db = CreateSenderDb(*db_data, std::move(params),
                               *(this->oprf_key_), 16, false);
  }

  CHECK_NULLPOINTER(sender_db, retcode::FAIL);
  uint32_t max_bin_bundles_per_bundle_idx = 0;
  uint32_t bundle_idx_count = sender_db->get_params().bundle_idx_count();
  for (uint32_t bundle_idx = 0; bundle_idx < bundle_idx_count; bundle_idx++) {
    max_bin_bundles_per_bundle_idx =
        std::max<uint32_t>(max_bin_bundles_per_bundle_idx,
                           sender_db->get_bin_bundle_count(bundle_idx));
  }
  ret = ProcessOprf();
  CHECK_RETCODE_WITH_RETVALUE(ret, retcode::FAIL);

  ret = ProcessQuery(sender_db);
  CHECK_RETCODE_WITH_RETVALUE(ret, retcode::FAIL);

  VLOG(5) << "end of execute task";
  return retcode::SUCCESS;
}

retcode KeywordPirOperatorServer::CreateDbDataCache(const DBData& db_data,
    std::unique_ptr<apsi::PSIParams> psi_params,
    apsi::oprf::OPRFKey& oprf_key,
    size_t nonce_byte_count,
    bool compress) {
  auto sender_db = CreateSenderDb(db_data, std::move(psi_params),
                                  oprf_key, nonce_byte_count, compress);
  if (sender_db == nullptr) {
    LOG(ERROR) << "create sender db failed";
    return retcode::FAIL;
  }
  std::fstream fout(this->options_.db_path, std::ios::out);
  size_t save_size = sender_db->save(fout);
  VLOG(0) << "save_size: " << save_size
          << " db path: " << this->options_.db_path;
  return retcode::SUCCESS;
}

auto KeywordPirOperatorServer::CreateSenderDb(const DBData &db_data,
    std::unique_ptr<PSIParams> psi_params,
    OPRFKey &oprf_key,
    size_t nonce_byte_count,
    bool compress) ->
    std::shared_ptr<SenderDB> {
  CHECK_TASK_STOPPED(nullptr);
  SCopedTimer timer;
  if (psi_params == nullptr) {
    LOG(ERROR) << "No Keyword pir parameters were given";
    return nullptr;
  }
  std::shared_ptr<SenderDB> sender_db{nullptr};
  if (holds_alternative<LabeledData>(db_data)) {
    VLOG(5) << "LabeledData";
    try {
      auto _start = timer.timeElapse();
      auto& labeled_db_data = std::get<LabeledData>(db_data);
      // Find the longest label and use that as label size
      size_t label_byte_count =
          std::max_element(labeled_db_data.begin(), labeled_db_data.end(),
            [](auto &a, auto &b) {
                return a.second.size() < b.second.size();
            })->second.size();

      auto max_label_count_ts = timer.timeElapse();
      VLOG(5) << "label_byte_count: " << label_byte_count << " "
              << "nonce_byte_count: " << nonce_byte_count << " "
              << "get max label count time cost(ms): " << max_label_count_ts;
      sender_db =
          std::make_shared<SenderDB>(*psi_params, label_byte_count,
                                     nonce_byte_count, compress);
      auto _mid = timer.timeElapse();
      VLOG(5) << "sender_db address: "
              << reinterpret_cast<uint64_t>(&(*sender_db));
      auto constuct_sender_db_time_cost = _mid - max_label_count_ts;
      sender_db->set_data(labeled_db_data);
      auto _end = timer.timeElapse();
      auto set_data_time_cost = _end - _mid;
      auto time_cost = _end - _start;
      VLOG(5) << "construct sender db time cost(ms): "
              << constuct_sender_db_time_cost << " "
              << "set sender db data time cost(ms): " << time_cost << " "
              << "total cost(ms): " << time_cost;
    } catch (const exception &ex) {
      LOG(ERROR) << "Failed to create keyword pir SenderDB: " << ex.what();
      return nullptr;
    }
  } else if (holds_alternative<UnlabeledData>(db_data)) {
    LOG(ERROR) << "Loaded keyword pir database is without label";
    return nullptr;
  } else {
    LOG(ERROR) << "Loaded keyword pir database is in an invalid state";
    return nullptr;
  }
  oprf_key = sender_db->get_oprf_key();
  auto time_cost = timer.timeElapse();
  VLOG(5) << "create_sender_db success, time cost(ms): " << time_cost;
  return sender_db;
}

std::unique_ptr<DBData> KeywordPirOperatorServer::CreateDb(
    const PirDataType& input) {
  auto result = LabeledData();
  result.reserve(input.size());
  std::string separator{DATA_RECORD_SEP};
  for (const auto& [item_str, label_vec] : input) {
    apsi::Item item = item_str;
    // label
    apsi::Label label;
    label.clear();
    for (size_t i = 0; i < label_vec.size() - 1; i++) {
      auto& label_str = label_vec[i];
      std::copy(label_str.begin(), label_str.end(), std::back_inserter(label));
      std::copy(separator.begin(), separator.end(), std::back_inserter(label));
    }
    auto& last_label = label_vec[label_vec.size() - 1];
    std::copy(last_label.begin(), last_label.end(), std::back_inserter(label));
    result.push_back(std::make_pair(std::move(item), std::move(label)));
  }
  return std::make_unique<DBData>(std::move(result));
}

// ------------------------Sender----------------------------
retcode KeywordPirOperatorServer::ProcessPSIParams() {
  CHECK_TASK_STOPPED(retcode::FAIL);
  std::string request_type_str;
  auto link_ctx = this->GetLinkContext();
  CHECK_NULLPOINTER_WITH_ERROR_MSG(link_ctx, "LinkContext is empty");
  auto ret = link_ctx->Recv(this->key_, ProxyNode(), &request_type_str);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "recv request from : " << PeerNode().to_string() << "failed";
    return retcode::FAIL;
  }
  ret = link_ctx->Send(this->key_, PeerNode(), psi_params_str_);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "send PSIParams to " << PeerNode().to_string() << "failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode KeywordPirOperatorServer::ProcessOprf() {
  CHECK_TASK_STOPPED(retcode::FAIL);
  VLOG(5) << "begin to process oprf";
  std::string oprf_request_str;
  auto link_ctx = this->GetLinkContext();
  auto ret = link_ctx->Recv(this->key_, this->ProxyNode(), &oprf_request_str);
  if (ret != retcode::SUCCESS || oprf_request_str.empty()) {
    LOG(ERROR) << "received oprf request from client failed ";
    return ret;
  }
  VLOG(5) << "received oprf request: " << oprf_request_str.size();

  // // OPRFKey key_oprf;
  auto oprf_response =
      OPRFSender::ProcessQueries(oprf_request_str, *(this->oprf_key_));

  std::string oprf_response_str{
      reinterpret_cast<char*>(const_cast<unsigned char*>(oprf_response.data())),
      oprf_response.size()};
  // VLOG(5) << "send data size: " << oprf_response_str.size() << " "
  //         << "data content: " << oprf_response_str;
  return link_ctx->Send(this->key_, PeerNode(), oprf_response_str);
}

retcode KeywordPirOperatorServer::ProcessQuery(
    std::shared_ptr<SenderDB> sender_db) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  CryptoContext crypto_context(sender_db->get_crypto_context());
  auto seal_context = sender_db->get_seal_context();
  auto query_request = std::make_unique<SenderOperationQuery>();

  std::string response_str;
  auto ret = this->GetLinkContext()->Recv(this->key_,
                                          this->ProxyNode(), &response_str);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "received response failed";
    return retcode::FAIL;
  }
  VLOG(5) << "received data length: " << response_str.size();
  if (response_str.empty()) {
    LOG(ERROR) << "received data can not be empty";
    return retcode::FAIL;
  }
  std::istringstream stream_in(response_str);
  try {
    query_request->load(stream_in, seal_context);
  } catch (std::exception& e) {
    LOG(ERROR) << "load response failed, " << e.what();
    return retcode::FAIL;
  }

  auto compr_mode = query_request->compr_mode;
  std::unordered_map<std::uint32_t, std::vector<seal::Ciphertext>> data_;
  for (auto& q : query_request->data) {
    VLOG(5) << "Extracting " << q.second.size()
            << " ciphertexts for exponent " << q.first;
    std::vector<Ciphertext> cts;
    for (auto &ct : q.second) {
      cts.push_back(ct.extract(seal_context));
      if (!is_valid_for(cts.back(), *seal_context)) {
          VLOG(5) << "Extracted ciphertext is invalid for SEALContext";
          return retcode::FAIL;
      }
    }
    data_[q.first] = std::move(cts);
  }
  VLOG(5) << "Start processing query request on database with "
        << sender_db->get_item_count() << " items";
  // auto& crypto_context = sender_db->get_crypto_context();

  seal::RelinKeys relin_keys_;
  apsi::PowersDag pd;
  crypto_context.set_evaluator(relin_keys_);
  // Copy over the CryptoContext from SenderDB;
  // set the Evaluator for this local instance.
  // Relinearization keys may not have been included in the query. In that case
  // query.relin_keys() simply holds an empty seal::RelinKeys instance.
  // There is no problem with the below call to CryptoContext::set_evaluator.
  // CryptoContext crypto_context(sender_db->get_crypto_context());
  // crypto_context.set_evaluator(query.relin_keys());

  // Get the PSIParams
  auto& params = sender_db->get_params();

  uint32_t bundle_idx_count = params.bundle_idx_count();
  uint32_t max_items_per_bin = params.table_params().max_items_per_bin;
  uint32_t ps_low_degree = params.query_params().ps_low_degree;
  const auto& query_powers = params.query_params().query_powers;
  auto target_powers = create_powers_set(ps_low_degree, max_items_per_bin);

  // Create the PowersDag
  pd.configure(query_powers, target_powers);

  // The query response only tells
  // how many ResultPackages to expect; send this first
  auto package_count = safe_cast<uint32_t>(sender_db->get_bin_bundle_count());
  // tell client how many package count need to receive
  std::string_view send_data{reinterpret_cast<char*>(&package_count),
                             sizeof(package_count)};
  auto link_ctx = this->GetLinkContext();
  ret = link_ctx->Send("package_count", PeerNode(), send_data);
  CHECK_RETCODE(ret);
  VLOG(5) << "package_countpackage_countpackage_count: " << package_count;
  // For each bundle index i, we need a vector of powers of the query Qᵢ.
  // We need powers all the way up to Qᵢ^max_items_per_bin.
  // We don't store the zeroth power. If Paterson-Stockmeyer is used,
  // then only a subset of the powers will be populated.
  std::vector<CiphertextPowers> all_powers(bundle_idx_count);
  auto pool = seal::MemoryManager::GetPool(mm_prof_opt::mm_force_new, true);
  // Initialize powers
  for (auto& powers : all_powers) {
    // The + 1 is because we index by power. The 0th power is a dummy value.
    // I promise this makes things easier to read.
    size_t powers_size = static_cast<size_t>(max_items_per_bin) + 1;
    powers.reserve(powers_size);
    for (size_t i = 0; i < powers_size; i++) {
      powers.emplace_back(pool);
    }
  }

  // Load inputs provided in the query
  for (auto &q : data_) {
    // The exponent of all the query powers we're about to iterate through
    size_t exponent = static_cast<size_t>(q.first);

    // Load Qᵢᵉ for all bundle indices i,
    // where e is the exponent specified above
    for (size_t bundle_idx = 0; bundle_idx < all_powers.size(); bundle_idx++) {
      // Load input^power to all_powers[bundle_idx][exponent]
      VLOG(5) << "Extracting query ciphertext power " << exponent
              << " for bundle index " << bundle_idx;
      all_powers[bundle_idx][exponent] = move(q.second[bundle_idx]);
    }
  }

  // Compute query powers for the bundle indexes
  for (size_t bundle_idx = 0; bundle_idx < bundle_idx_count; bundle_idx++) {
    ComputePowers(sender_db, crypto_context, all_powers, pd,
                  bundle_idx, pool);
  }

  VLOG(5) << "Finished computing powers for all bundle indices";
  VLOG(5) << "Start processing bin bundle caches";
  primihub::ThreadSafeQueue<std::string> result_package_queue;
  std::vector<future<void>> futures;
  for (uint32_t bundle_idx = 0; bundle_idx < bundle_idx_count; bundle_idx++) {
    auto bundle_caches = sender_db->get_cache_at(bundle_idx);
    for (auto &cache : bundle_caches) {
      futures.push_back(
        std::async(
          std::launch::async,
          [&, bundle_idx, cache, this]() -> void {
            auto result_package =
                ProcessBinBundleCache(sender_db,
                                      crypto_context,
                                      cache,
                                      all_powers,
                                      static_cast<uint32_t>(bundle_idx),
                                      compr_mode,
                                      pool);
            if (result_package == nullptr) {
              return;
            }
            // serialize and push into result package queue
            std::ostringstream string_ss;
            result_package->save(string_ss);
            std::string result_package_str = string_ss.str();
            size_t data_len = result_package_str.length();
            result_package_queue.push(std::move(result_package_str));
            VLOG(5) << "push data into result package queue, "
                    << "data length: " << data_len
                    << " label_result size: "
                    << result_package->label_result.size();
          }));
    }
  }

  futures.push_back(
    std::async(
      std::launch::async,
      [&, this]() -> void {
        auto link_ctx = this->GetLinkContext();
        VLOG(5) << "package_count: " << package_count;
        for (size_t i = 0; i < package_count; i++) {
          std::string send_data;
          result_package_queue.wait_and_pop(send_data);
          auto ret = link_ctx->Send(this->key_, PeerNode(), send_data);
          if (ret != retcode::SUCCESS) {
            LOG(ERROR) << "send result to client, index: " << i
                << " data length: " << send_data.size() << " failed";
            return;
          }
          VLOG(5) << "send result to client, index: " << i
                  << " data length: " << send_data.size();
        }
      }));
  // Wait until all bin bundle caches have been processed
  for (auto& f : futures) {
    f.get();
  }

  VLOG(5) << "Finished processing query request";
  return retcode::SUCCESS;
}

retcode KeywordPirOperatorServer::ComputePowers(
    const shared_ptr<SenderDB> &sender_db,
    const apsi::CryptoContext &crypto_context,
    std::vector<CiphertextPowers> &all_powers,
    const apsi::PowersDag &pd,
    uint32_t bundle_idx,
    seal::MemoryPoolHandle &pool) {
  CHECK_TASK_STOPPED(retcode::FAIL);
  STOPWATCH(sender_stopwatch, "Sender::ComputePowers");
  auto bundle_caches = sender_db->get_cache_at(bundle_idx);
  if (!bundle_caches.size()) {
    return retcode::FAIL;
  }
  // Compute all powers of the query
  VLOG(5) << "Computing all query ciphertext powers for bundle index "
          << bundle_idx;

  auto evaluator = crypto_context.evaluator();
  auto relin_keys = crypto_context.relin_keys();
  CiphertextPowers &powers_at_this_bundle_idx = all_powers[bundle_idx];
  bool relinearize = crypto_context.seal_context()->using_keyswitching();
  pd.parallel_apply([&](const PowersDag::PowersNode &node) {
    if (!node.is_source()) {
      auto parents = node.parents;
      Ciphertext prod(pool);
      if (parents.first == parents.second) {
        evaluator->square(powers_at_this_bundle_idx[parents.first], prod, pool);
      } else {
        evaluator->multiply(
            powers_at_this_bundle_idx[parents.first],
            powers_at_this_bundle_idx[parents.second],
            prod,
            pool);
      }
      if (relinearize) {
        evaluator->relinearize_inplace(prod, *relin_keys, pool);
      }
      powers_at_this_bundle_idx[node.power] = move(prod);
    }
  });

  // Now that all powers of the ciphertext have been computed,
  // we need to transform them to NTT form.
  // This will substantially improve the polynomial evaluation,
  // because the plaintext polynomials are already in NTT transformed form,
  // and the ciphertexts are used repeatedly for each bin bundle at this index.
  // This computation is separate from the graph processing above,
  // because the multiplications must all be
  // done before transforming to NTT form.
  // We omit the first ciphertext in the vector,
  // because it corresponds to the zeroth power of the query
  // and is included only for convenience of the indexing;
  // the ciphertext is actually not set or valid for use.

  ThreadPoolMgr tpm;

  // After computing all powers we will modulus switch down to parameters
  // that one more level for low powers than for high powers;
  // same choice must be used when encoding/NTT transforming the SenderDB data.
  auto high_powers_parms_id =
      get_parms_id_for_chain_idx(*crypto_context.seal_context(), 1);
  auto low_powers_parms_id =
      get_parms_id_for_chain_idx(*crypto_context.seal_context(), 2);

  uint32_t ps_low_degree = sender_db->get_params().query_params().ps_low_degree;

  vector<future<void>> futures;
  for (uint32_t power : pd.target_powers()) {
    futures.push_back(tpm.thread_pool().enqueue([&, power]() {
      if (!ps_low_degree) {
        // Only one ciphertext-plaintext multiplication is needed after this
        evaluator->mod_switch_to_inplace(
            powers_at_this_bundle_idx[power], high_powers_parms_id, pool);

        // All powers must be in NTT form
        evaluator->transform_to_ntt_inplace(powers_at_this_bundle_idx[power]);
      } else {
        if (power <= ps_low_degree) {
          // Low powers must be at a higher level than high powers
          evaluator->mod_switch_to_inplace(
              powers_at_this_bundle_idx[power], low_powers_parms_id, pool);

          // Low powers must be in NTT form
          evaluator->transform_to_ntt_inplace(powers_at_this_bundle_idx[power]);
        } else {
          // High powers are only modulus switched
          evaluator->mod_switch_to_inplace(
              powers_at_this_bundle_idx[power], high_powers_parms_id, pool);
        }
      }
    }));
  }

  for (auto &f : futures) {
    f.get();
  }
  return retcode::SUCCESS;
}

auto KeywordPirOperatorServer::ProcessBinBundleCache(
    const shared_ptr<apsi::sender::SenderDB> &sender_db,
    const apsi::CryptoContext &crypto_context,
    reference_wrapper<const apsi::sender::BinBundleCache> cache,
    std::vector<apsi::sender::CiphertextPowers> &all_powers,
    uint32_t bundle_idx,
    compr_mode_type compr_mode,
    seal::MemoryPoolHandle &pool) ->
    std::unique_ptr<apsi::network::ResultPackage> {
  CHECK_TASK_STOPPED(nullptr);
  // Package for the result data
  auto rp = std::make_unique<apsi::network::ResultPackage>();
  rp->compr_mode = compr_mode;

  rp->bundle_idx = bundle_idx;
  rp->nonce_byte_count = safe_cast<uint32_t>(sender_db->get_nonce_byte_count());
  rp->label_byte_count = safe_cast<uint32_t>(sender_db->get_label_byte_count());

  // Compute the matching result and move to rp
  // const apsi::sender::BatchedPlaintextPolyn
  const auto& matching_polyn = cache.get().batched_matching_polyn;

  // Determine if we use Paterson-Stockmeyer or not
  uint32_t ps_low_degree = sender_db->get_params().query_params().ps_low_degree;
  auto degree = safe_cast<uint32_t>(matching_polyn.batched_coeffs.size()) - 1;
  bool using_ps = (ps_low_degree > 1) && (ps_low_degree < degree);
  if (using_ps) {
    rp->psi_result = matching_polyn.eval_patstock(
        crypto_context, all_powers[bundle_idx],
        safe_cast<size_t>(ps_low_degree), pool);
  } else {
    rp->psi_result = matching_polyn.eval(all_powers[bundle_idx], pool);
  }

  for (const auto &interp_polyn : cache.get().batched_interp_polyns) {
    // Compute the label result and move to rp
    degree = safe_cast<uint32_t>(interp_polyn.batched_coeffs.size()) - 1;
    using_ps = (ps_low_degree > 1) && (ps_low_degree < degree);
    if (using_ps) {
      rp->label_result.push_back(
          interp_polyn.eval_patstock(
              crypto_context, all_powers[bundle_idx], ps_low_degree, pool));
    } else {
      rp->label_result.push_back(
          interp_polyn.eval(all_powers[bundle_idx], pool));
    }
  }
  VLOG(5) << "get ResultPackage for bundle_idx:  " << bundle_idx;
  return rp;
}

std::unique_ptr<apsi::PSIParams> KeywordPirOperatorServer::SetPsiParams() {
  CHECK_TASK_STOPPED(nullptr);
  std::string params_json;
  std::string pir_server_config_path{"config/pir_server_config.json"};
  VLOG(5) << "pir_server_config_path: " << pir_server_config_path;
  try {
    // throw_if_file_invalid(cmd.params_file());
    std::fstream input_file(pir_server_config_path, std::ios_base::in);
    if (!input_file.is_open()) {
      LOG(ERROR) << "open " << pir_server_config_path << " failed";
      return nullptr;
    }
    std::string line;
    while (getline(input_file, line)) {
      params_json.append(line);
      params_json.append("\n");
    }
    input_file.close();
  } catch (const std::exception &ex) {
    LOG(ERROR) << "Error trying to read input file "
               << pir_server_config_path << ": " << ex.what();
    return nullptr;
  }

  // std::unique_ptr<PSIParams> params{nullptr};
  auto params = std::make_unique<PSIParams>(PSIParams::Load(params_json));
  SCopedTimer timer;
  std::ostringstream param_ss;
  size_t param_size = params->save(param_ss);
  psi_params_str_ = param_ss.str();
  auto time_cost = timer.timeElapse();
  VLOG(5) << "param_size: " << param_size << " time cost(ms): " << time_cost;
  VLOG(5) << "param_content: " << psi_params_str_.size();
  return params;
}

bool KeywordPirOperatorServer::DbCacheAvailable(const std::string& db_path) {
  return FileExists(db_path);
}

std::shared_ptr<apsi::sender::SenderDB>
KeywordPirOperatorServer::LoadDbFromCache(const std::string& db_file_cache) {
  SCopedTimer timer;
  std::fstream fin(db_file_cache, std::ios::in);
  auto db_info = SenderDB::Load(fin);
  auto sender_db = std::make_shared<SenderDB>(std::move(std::get<0>(db_info)));
  VLOG(0) << "load data from cache file, db_size: " << std::get<1>(db_info);
  *(this->oprf_key_) = sender_db->get_oprf_key();
  auto time_cost = timer.timeElapse();
  VLOG(5) << "LoadDbFromCache cost time(ms): " << time_cost;
  return sender_db;
}

}  // namespace primihub::pir
