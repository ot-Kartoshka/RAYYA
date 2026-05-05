#pragma once

#include <concepts>
#include <cstdint>
#include <expected>

#include "Errors.hpp"


template<std::unsigned_integral T>
constexpr T modmul(T a, T b, T mod) noexcept {
    return static_cast<T>( (static_cast<__uint128_t>(a) % mod) * (static_cast<__uint128_t>(b) % mod) % static_cast<__uint128_t>(mod) );
}

template<std::unsigned_integral T>
constexpr T modpow(T base, T exp, T mod) noexcept {
    if (mod == T{ 1 }) return T{ 0 };
    T result{ 1 };
    base %= mod;
    while (exp > T{ 0 }) {
        if (exp & T{ 1 }) result = modmul(result, base, mod);
        base = modmul(base, base, mod);
        exp >>= T{ 1 };
    }
    return result;
}


uint64_t modinv(uint64_t a, uint64_t m) noexcept;

bool miller_rabin(uint64_t n) noexcept;

bool is_primitive_root(uint64_t alpha, uint64_t p) noexcept;

std::expected<uint64_t, Error> parse_number(std::string_view s) noexcept;