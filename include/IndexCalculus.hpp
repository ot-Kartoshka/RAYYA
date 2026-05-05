#pragma once

#include <cstdint>
#include <expected>

#include "Errors.hpp"


struct DLPResult {
    uint64_t value;
    double elapsed_ms;
    size_t relations_count;
    unsigned threads_used;
};


std::expected<DLPResult, Error> index_calculus( uint64_t p, uint64_t alpha, uint64_t beta );


std::expected<DLPResult, Error> index_calculus_par( uint64_t p, uint64_t alpha, uint64_t beta, unsigned num_threads = 1 );