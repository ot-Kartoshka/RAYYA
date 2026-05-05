#include "../include/IndexCalculus.hpp"
#include "../include/NumberUtils.hpp"

#include <atomic>
#include <print>
#include <chrono>
#include <cmath>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <vector>


using Clock = std::chrono::steady_clock;

static double elapsed_ms(Clock::time_point start) noexcept {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

static std::vector<uint64_t> build_factor_base(uint64_t n) {
    const uint64_t B = static_cast<uint64_t>( 3.38 * std::exp(0.5 * std::sqrt(std::log(n) * std::log(std::log(n)))));

    std::vector<bool> is_prime(static_cast<size_t>(B + 1u), true);
    is_prime[0] = is_prime[1] = false;
    for (uint64_t i = 2u; i * i <= B; ++i)
        if (is_prime[static_cast<size_t>(i)])
            for (uint64_t j = i * i; j <= B; j += i)
                is_prime[static_cast<size_t>(j)] = false;

    std::vector<uint64_t> primes;
    for (uint64_t i = 2u; i <= B; ++i)
        if (is_prime[static_cast<size_t>(i)]) primes.push_back(i);
    return primes;
}



static std::optional<std::vector<int>> try_smooth( uint64_t value, const std::vector<uint64_t>& base) {
    std::vector<int> exponents(base.size(), 0);
    for (size_t i = 0u; i < base.size(); ++i) {
        while (value % base[i] == 0u) {
            ++exponents[i];
            value /= base[i];
        }
        if (value == 1u) return exponents;
    }
    return value == 1u ? std::optional{ exponents } : std::nullopt;
}



static std::optional<std::vector<uint64_t>> solve_mod( std::vector<std::vector<int64_t>> A, std::vector<int64_t> b, size_t x, uint64_t n) {

    const size_t rows = A.size();
    std::vector<int> pivot_col(rows, -1);
    int current = 0;

    for (size_t col = 0u; col < x && current < static_cast<int>(rows); ++col) {

        int found = -1;
        for (int r = current; r < static_cast<int>(rows); ++r) {
            const int64_t v = A[static_cast<size_t>(r)][col] % static_cast<int64_t>(n);
            const uint64_t u = static_cast<uint64_t>(v < 0 ? v + static_cast<int64_t>(n) : v);
            if (u != 0u && std::gcd(u, n) == 1u) { found = r; break; }
        }
        if (found == -1) continue;

        std::swap(A[static_cast<size_t>(found)], A[static_cast<size_t>(current)]);
        std::swap(b[static_cast<size_t>(found)], b[static_cast<size_t>(current)]);


        int64_t raw = A[static_cast<size_t>(current)][col] % static_cast<int64_t>(n);
        uint64_t piv = static_cast<uint64_t>(raw < 0 ? raw + static_cast<int64_t>(n) : raw);
        const uint64_t inv = modinv(piv, n);

        for (size_t j = 0u; j < x; ++j) {
            int64_t val = A[static_cast<size_t>(current)][j] % static_cast<int64_t>(n);
            if (val < 0) val += static_cast<int64_t>(n);
            A[static_cast<size_t>(current)][j] = static_cast<int64_t>( static_cast<__uint128_t>(static_cast<uint64_t>(val)) * inv % n);
        }
        int64_t bval = b[static_cast<size_t>(current)] % static_cast<int64_t>(n);
        if (bval < 0) bval += static_cast<int64_t>(n);
        b[static_cast<size_t>(current)] = static_cast<int64_t>( static_cast<__uint128_t>(static_cast<uint64_t>(bval)) * inv % n);

        for (int r = 0; r < static_cast<int>(rows); ++r) {
            if (r == current) continue;
            int64_t factor = A[static_cast<size_t>(r)][col] % static_cast<int64_t>(n);
            if (factor < 0) factor += static_cast<int64_t>(n);
            if (factor == 0) continue;

            for (size_t j = 0u; j < x; ++j) {
                int64_t cur = A[static_cast<size_t>(r)][j] % static_cast<int64_t>(n);
                if (cur < 0) cur += static_cast<int64_t>(n);
                const uint64_t sub = static_cast<uint64_t>( static_cast<__uint128_t>(static_cast<uint64_t>(factor)) * static_cast<uint64_t>(A[static_cast<size_t>(current)][j]) % n);
                A[static_cast<size_t>(r)][j] = static_cast<int64_t>( (static_cast<uint64_t>(cur) + n - sub) % n);
            }

            int64_t bcur = b[static_cast<size_t>(r)] % static_cast<int64_t>(n);
            if (bcur < 0) bcur += static_cast<int64_t>(n);
            const uint64_t bsub = static_cast<uint64_t>( static_cast<__uint128_t>(static_cast<uint64_t>(factor)) * static_cast<uint64_t>(b[static_cast<size_t>(current)]) % n);
            b[static_cast<size_t>(r)] = static_cast<int64_t>( (static_cast<uint64_t>(bcur) + n - bsub) % n);
        }

        pivot_col[static_cast<size_t>(current)] = static_cast<int>(col);
        ++current;
    }

    std::vector<uint64_t> solution(x, 0u);
    for (int r = 0; r < current; ++r) {
        const int col = pivot_col[static_cast<size_t>(r)];
        if (col >= 0) {
            int64_t bval = b[static_cast<size_t>(r)] % static_cast<int64_t>(n);
            if (bval < 0) bval += static_cast<int64_t>(n);
            solution[static_cast<size_t>(col)] = static_cast<uint64_t>(bval);
        }
    }

    for (size_t i = 0u; i < rows; ++i) {
        __uint128_t lhs = 0u;
        for (size_t j = 0u; j < x; ++j) {
            int64_t raw = A[i][j] % static_cast<int64_t>(n);
            if (raw < 0) raw += static_cast<int64_t>(n);
            lhs = (lhs + static_cast<__uint128_t>(static_cast<uint64_t>(raw)) *
                solution[j]) % n;
        }
        int64_t bval = b[i] % static_cast<int64_t>(n);
        if (bval < 0) bval += static_cast<int64_t>(n);
        if (lhs != static_cast<uint64_t>(bval)) return std::nullopt;
    }

    return solution;
}


struct Relation {
    std::vector<int64_t> coef;
    int64_t rhs;
};

static void collect_one( uint64_t p, uint64_t alpha, uint64_t n, const std::vector<uint64_t>& base, std::mt19937_64& rng, std::vector<Relation>& out) {

    const uint64_t k = std::uniform_int_distribution<uint64_t>(0u, n - 1u)(rng);
    const uint64_t alpha_k = modpow(alpha, k, p);
    auto smooth = try_smooth(alpha_k, base);
    if (!smooth) return;

    Relation rel;
    rel.coef.resize(base.size());
    for (size_t i = 0u; i < base.size(); ++i) rel.coef[i] = static_cast<int64_t>((*smooth)[i]);
    rel.rhs = static_cast<int64_t>(k);
    out.push_back(std::move(rel));
}


static std::optional<uint64_t> DLP( uint64_t p, uint64_t alpha, uint64_t beta, uint64_t n, const std::vector<uint64_t>& base, const std::vector<uint64_t>& log_table, std::mt19937_64& rng) {

    const size_t max_tries = 200000u;
    for (size_t attempt = 0u; attempt < max_tries; ++attempt) {
        const uint64_t l = std::uniform_int_distribution<uint64_t>(0u, n - 1u)(rng);
        const uint64_t candidate = static_cast<uint64_t>( static_cast<__uint128_t>(beta) * modpow(alpha, l, p) % p);

        auto smooth = try_smooth(candidate, base);
        if (!smooth) continue;

        __uint128_t sum = 0u;
        for (size_t i = 0u; i < base.size(); ++i) sum = (sum + static_cast<__uint128_t>((*smooth)[i]) * log_table[i]) % n;

        const uint64_t result = static_cast<uint64_t>((sum + n - l % n) % n);
        if (modpow(alpha, result, p) == beta) return result;
    }
    return std::nullopt;
}




std::expected<DLPResult, Error> index_calculus( uint64_t p, uint64_t alpha, uint64_t beta) {

    if (p < 3u || !miller_rabin(p)) return std::unexpected(Error::InvalidInput);
	if (!is_primitive_root(alpha, p)) return std::unexpected(Error::InvalidGenerator); //=> якщо не буде, СЛР треба робити по модулю генератора альфа і перевірка чи взагалі бета лежить всередині групи яку генерує альфа => пошук порядку альфа(на вхід не подається, задача факторизації), перевірка чи бета належить групі генератора альфа: beta ^ ord(alpha) = 1 (mod p). + метожичны матерыалу щодо входу прямо вказують що альфа генератор групи
    if (beta == 0u || beta >= p) return std::unexpected(Error::InvalidInput);

    const auto start = Clock::now();
    const uint64_t n = p - 1u;
    const auto base = build_factor_base(n);
    const size_t t = base.size();
    const size_t target = t + 15u;

    std::mt19937_64 rng(std::random_device{}());
    std::vector<Relation> relations;
    relations.reserve(target);

    while (relations.size() < target) collect_one(p, alpha, n, base, rng, relations);

    std::vector<std::vector<int64_t>> A(relations.size(), std::vector<int64_t>(t));
    std::vector<int64_t> b(relations.size());
    for (size_t i = 0u; i < relations.size(); ++i) {
        A[i] = relations[i].coef;
        b[i] = relations[i].rhs;
    }

    const auto maybe_logs = solve_mod(A, b, t, n);
    if (!maybe_logs) return std::unexpected(Error::SystemUnsolvable);

    const auto maybe_result = DLP(p, alpha, beta, n, base, *maybe_logs, rng);
    if (!maybe_result) return std::unexpected(Error::LogarithmNotFound);

    return DLPResult{
        .value = *maybe_result,
        .elapsed_ms = elapsed_ms(start),
        .relations_count = relations.size(),
        .threads_used = 1u,
    };
}


std::expected<DLPResult, Error> index_calculus_par( uint64_t p, uint64_t alpha, uint64_t beta, unsigned num_threads) {

    if (p < 3u || !miller_rabin(p)) return std::unexpected(Error::InvalidInput);
    if (!is_primitive_root(alpha, p)) return std::unexpected(Error::InvalidGenerator);
    if (beta == 0u || beta >= p) return std::unexpected(Error::InvalidInput);

    const auto start = Clock::now();
    const uint64_t n = p - 1u;
    const auto base = build_factor_base(n);
    const size_t t = base.size();
    const size_t target = t + 15u;

    const unsigned hw_threads = std::thread::hardware_concurrency();
    const unsigned max_useful = (hw_threads > 0u) ? hw_threads : 4u;

    if (num_threads == 0u) {
        num_threads = max_useful;
    }
    else if (num_threads > max_useful) {
        std::println("  Запрошено {} потоків, але CPU підтримує лише {}. " "Зменшено до {}.", num_threads, max_useful, max_useful);
        num_threads = max_useful;
    }

    std::vector<Relation> relations;
    std::mutex pool_mutex;
    std::atomic<bool> done{ false };

    auto worker = [&](unsigned id) {
        std::mt19937_64 rng(std::random_device{}() ^ (static_cast<uint64_t>(id) * 6364136223846793005ULL));
        std::vector<Relation> local;

        while (!done.load(std::memory_order_relaxed)) {
            collect_one(p, alpha, n, base, rng, local);
            if (local.empty()) continue;

            std::lock_guard lock(pool_mutex);
            for (auto& rel : local) relations.push_back(std::move(rel));
            local.clear();

            if (relations.size() >= target) done.store(true, std::memory_order_relaxed);
        }
        };

    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    for (unsigned id = 0u; id < num_threads; ++id) workers.emplace_back(worker, id);
    for (auto& w : workers) w.join();

    if (relations.size() > target) relations.resize(target);

    std::vector<std::vector<int64_t>> A(relations.size(), std::vector<int64_t>(t));
    std::vector<int64_t> b(relations.size());
    for (size_t i = 0u; i < relations.size(); ++i) {
        A[i] = relations[i].coef;
        b[i] = relations[i].rhs;
    }

    const auto maybe_logs = solve_mod(A, b, t, n);
    if (!maybe_logs) return std::unexpected(Error::SystemUnsolvable);

    std::mt19937_64 rng(std::random_device{}());
    const auto maybe_result = DLP(p, alpha, beta, n, base, *maybe_logs, rng);
    if (!maybe_result) return std::unexpected(Error::LogarithmNotFound);

    return DLPResult{
        .value = *maybe_result,
        .elapsed_ms = elapsed_ms(start),
        .relations_count = relations.size(),
        .threads_used = num_threads,
    };
}