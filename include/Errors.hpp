#pragma once

#include <string_view>


enum class Error {
    InvalidInput,
    InvalidGenerator,
    NoRelationsFound,
    SystemUnsolvable,
    LogarithmNotFound,
};

std::string_view error_to_string(Error err);