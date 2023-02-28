
// Copyright [2022] <primihub>
#ifndef SRC_PRIMIHUB_SERVICE_ERROR_HPP_
#define SRC_PRIMIHUB_SERVICE_ERROR_HPP_

#include "src/primihub/service/outcome_reg.hpp"

namespace primihub::service {
enum class Error {
    NO_ERROR = 0,
    NO_PEERS,
    MESSAGE_PARSE_ERROR,
    MESSAGE_DESERIALIZE_ERROR,
    MESSAGE_SERIALIZE_ERROR,
    MESSAGE_WRONG,
    UNEXPECTED_MESSAGE_TYPE,
    STREAM_RESET,
    VALUE_NOT_FOUND,
    CONTENT_VALIDATION_FAILED,
    TIMEOUT,
    IN_PROGRESS,
    FULFILLED,
    NOT_IMPLEMENTED,
    INTERNAL_ERROR,
    SESSION_CLOSED,
    VALUE_MISMATCH
};

}  // namespace primihub::service

OUTCOME_HPP_DECLARE_ERROR(primihub::service, Error);

#endif  // SRC_PRIMIHUB_SERVICE_ERROR_HPP_
