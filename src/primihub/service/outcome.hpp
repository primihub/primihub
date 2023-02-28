
#ifndef SRC_PRIMIHUB_SERVICE_OUTCOME_HPP_
#define SRC_PRIMIHUB_SERVICE_OUTCOME_HPP_

#include <boost/outcome/result.hpp>
#include <boost/outcome/success_failure.hpp>
#include <boost/outcome/try.hpp>

#define OUTCOME_TRY(...) BOOST_OUTCOME_TRY(__VA_ARGS__)

namespace primihub::service::outcome {
using namespace BOOST_OUTCOME_V2_NAMESPACE;  // NOLINT
template <class R, class S = std::error_code,
          class NoValuePolicy = policy::default_policy<R, S, void>>  //

using result = basic_result<R, S, NoValuePolicy>;
}  // namespace primihub::service::outcome

#endif  // SRC_PRIMIHUB_SERVICE_OUTCOME_HPP_
