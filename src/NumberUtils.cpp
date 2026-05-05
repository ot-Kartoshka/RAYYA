#include "../include/NumberUtils.hpp"

#include <array>
#include <cctype>
#include <vector>


uint64_t modinv(uint64_t a, uint64_t m) noexcept {

    __int128 r0 = static_cast<__int128>(m), r1 = static_cast<__int128>(a % m);
    __int128 s0 = 0, s1 = 1;
    while (r1 != 0) {
        const __int128 q = r0 / r1;
        const __int128 r2 = r0 - q * r1;
        const __int128 s2 = s0 - q * s1;
        r0 = r1; r1 = r2;
        s0 = s1; s1 = s2;
    }
    if (s0 < 0) s0 += static_cast<__int128>(m);
    return static_cast<uint64_t>(s0);
}


static bool mr_round(uint64_t n, uint64_t a, uint64_t d, int s) noexcept {

    uint64_t x = modpow(a, d, n);
    if (x == 1u || x == n - 1u) return true;
    for (int r = 1; r < s; ++r) {
        x = modmul(x, x, n);
        if (x == n - 1u) return true;
        if (x == 1u)     return false;
    }
    return false;
}

bool miller_rabin(uint64_t n) noexcept {

    if (n < 2u) return false;
    if (n == 2u || n == 3u) return true;
    if ((n & 1u) == 0u) return false;

    uint64_t d = n - 1u;
    int s = 0;
    while ((d & 1u) == 0u) { d >>= 1u; ++s; }

    static constexpr std::array<uint64_t, 12> witnesses{
        2u, 3u, 5u, 7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u, 37u
    };
    for (const uint64_t a : witnesses) {
        if (a >= n) continue;
        if (!mr_round(n, a, d, s)) return false;
    }
    return true;
}


bool is_primitive_root(uint64_t alpha, uint64_t p) noexcept {
    if (alpha == 0u || alpha >= p) return false;
    uint64_t order = p - 1u;
    uint64_t tmp = order;
    for (uint64_t d = 2u; d * d <= tmp; ++d) {
        if (tmp % d == 0u) {
            if (modpow(alpha, order / d, p) == 1u) return false;
            while (tmp % d == 0u) tmp /= d;
        }
    }
    if (tmp > 1u && modpow(alpha, order / tmp, p) == 1u) return false;
    return true;
}


std::expected<uint64_t, Error> parse_number(std::string_view s) noexcept {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
        s.remove_prefix(1u);
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
        s.remove_suffix(1u);
    if (s.empty()) return std::unexpected(Error::InvalidInput);

    int base = 10;
    if (s.size() >= 2u && s[0] == '0') {
        if (s[1] == 'x' || s[1] == 'X') { base = 16; s.remove_prefix(2u); }
        else if (s[1] == 'b' || s[1] == 'B') { base = 2;  s.remove_prefix(2u); }
    }
    if (s.empty()) return std::unexpected(Error::InvalidInput);

    __uint128_t value = 0u;
    constexpr __uint128_t kMax = static_cast<__uint128_t>(UINT64_MAX);
    for (const char c : s) {
        int digit = -1;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (base == 16 && c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (base == 16 && c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        if (digit < 0 || digit >= base) return std::unexpected(Error::InvalidInput);
        value = value * static_cast<unsigned>(base) + static_cast<unsigned>(digit);
        if (value > kMax) return std::unexpected(Error::InvalidInput);
    }
    return static_cast<uint64_t>(value);
}