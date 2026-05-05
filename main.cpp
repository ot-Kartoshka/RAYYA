#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <print>
#include <string>
#include <thread>
#include <vector>

#include "./include/Errors.hpp"
#include "./include/IndexCalculus.hpp"
#include "./include/NumberUtils.hpp"


using Clock = std::chrono::steady_clock;

static std::size_t utf8_width(const std::string& s) noexcept {
    std::size_t n = 0u;
    for (unsigned char c : s)
        if ((c & 0xC0u) != 0x80u) ++n;
    return n;
}

static std::string make_bullets(std::size_t n) {
    std::string r;
    r.reserve(4u * n);
    for (std::size_t i = 0u; i < n; ++i) {
        if (i) r += ' ';
        r += "•";
    }
    return r;
}

static std::string make_border(std::size_t W) {
    return make_bullets((W + 1u) / 2u);
}

static std::string str_center(const std::string& s, std::size_t w) {
    const std::size_t v = utf8_width(s);
    if (v >= w) return s;
    const std::size_t pad = w - v;
    return std::string(pad / 2u, ' ') + s + std::string(pad - pad / 2u, ' ');
}

static std::string str_ljust(const std::string& s, std::size_t w) {
    const std::size_t v = utf8_width(s);
    return v < w ? s + std::string(w - v, ' ') : s;
}

static std::pair<std::string, std::size_t> make_section_title(
    const std::string& title, std::size_t min_w = 0u)
{
    const std::size_t tv = utf8_width(title);
    const std::size_t min_bullets = 3u;
    const std::size_t L = std::max(min_bullets, min_w > tv ? (min_w - tv + 4u) / 4u : min_bullets);
    std::size_t W = 4u * L + tv;
    const std::string extra = (W % 2u == 0u) ? " " : "";
    if (W % 2u == 0u) ++W;
    return { make_bullets(L) + ' ' + extra + title + ' ' + make_bullets(L), W };
}


static std::size_t print_dlog_input(uint64_t p, uint64_t alpha, uint64_t beta) {
    const std::string sp = "  p     =  " + std::to_string(p);
    const std::string sa = "  alpha =  " + std::to_string(alpha);
    const std::string sb = "  beta  =  " + std::to_string(beta);
    const std::size_t cw = std::max({ utf8_width(sp), utf8_width(sa), utf8_width(sb) });

    auto [title_line, W] = make_section_title("DLP: ", cw);
    std::println("{}", title_line);
    std::println("{}", sp);
    std::println("{}", sa);
    std::println("{}", sb);
    std::println("{}", make_border(W));
    return W;
}

static void print_result_block( const std::string& algo_title, uint64_t p, uint64_t alpha, uint64_t beta, const std::expected<DLPResult, Error>& result, std::size_t min_w) {
    auto [title_line, W] = make_section_title(algo_title, min_w);
    std::println("{}", title_line);

    if (!result) {
        std::println("  Помилка : {}", error_to_string(result.error()));
        std::println("{}", make_border(W));
        return;
    }

    const auto& res = *result;
    const uint64_t check = modpow(alpha, res.value, p);
    const bool correct = (check == beta);

    std::println("  x              =  {}", res.value);
    std::println("  час            =  {:.3f} мс", res.elapsed_ms);
    std::println("  співвідношень  =  {}", res.relations_count);
    std::println("  потоків        =  {}", res.threads_used);
    std::println("  {}^{} mod {} = {}  {}", alpha, res.value, p, check, correct ? "(вірно)" : "(ПОМИЛКА!)");
    std::println("{}", make_border(W));

    if (correct)
        std::println("  -> відповідь: log_{}({}) mod {} = {}", alpha, beta, p, res.value);
}


static int run_seq(uint64_t p, uint64_t alpha, uint64_t beta) {
    const std::size_t in = print_dlog_input(p, alpha, beta);
    const auto result = index_calculus(p, alpha, beta);
    print_result_block("Index-calculus (послідовний)", p, alpha, beta, result, in);
    return result ? 0 : 1;
}



static int run_par(uint64_t p, uint64_t alpha, uint64_t beta, unsigned threads) {
    const std::size_t in = print_dlog_input(p, alpha, beta);
    const auto result = index_calculus_par(p, alpha, beta, threads);
    const std::string title = "Index-calculus (паралельний, " + std::to_string(result ? result->threads_used : threads) + " потоки/ів)";
    print_result_block(title, p, alpha, beta, result, in);
    return result ? 0 : 1;
}




static int run_both(uint64_t p, uint64_t alpha, uint64_t beta, unsigned threads) {
    const std::size_t in = print_dlog_input(p, alpha, beta);

    const auto seq_result = index_calculus(p, alpha, beta);
    const auto par_result = index_calculus_par(p, alpha, beta, threads);

    const std::string h_algo = "Алгоритм";
    const std::string h_ans = "Відповідь";
    const std::string h_time = "Час (мс)";
    const std::string h_rel = "Співвідношень";

    const std::string algo_seq = "Послідовний";
    const std::string algo_par = "Паралельний (" + std::to_string(par_result ? par_result->threads_used : std::max(1u, std::thread::hardware_concurrency())) + " потоки/ів)";

    char t_seq[32], t_par[32];
    std::snprintf(t_seq, sizeof(t_seq), "%.3f", seq_result ? seq_result->elapsed_ms : 0.0);
    std::snprintf(t_par, sizeof(t_par), "%.3f", par_result ? par_result->elapsed_ms : 0.0);

    const std::string ans_seq = seq_result ? std::to_string(seq_result->value) : "(помилка)";
    const std::string ans_par = par_result ? std::to_string(par_result->value) : "(помилка)";
    const std::string rel_seq = seq_result ? std::to_string(seq_result->relations_count) : "-";
    const std::string rel_par = par_result ? std::to_string(par_result->relations_count) : "-";

    const std::size_t w_algo = std::max({ utf8_width(h_algo), utf8_width(algo_seq), utf8_width(algo_par) });
    const std::size_t w_ans = std::max({ utf8_width(h_ans),  utf8_width(ans_seq),  utf8_width(ans_par) });
    const std::size_t w_time = std::max({ utf8_width(h_time), utf8_width(t_seq),    utf8_width(t_par) });
    const std::size_t w_rel = std::max({ utf8_width(h_rel),  utf8_width(rel_seq),  utf8_width(rel_par) });

    const std::size_t row_w = w_algo + w_ans + w_time + w_rel + 9u;
    auto [title_line, W] = make_section_title("Порівняння алгоритмів", std::max(in, row_w + 4u));

    const std::size_t margin = W > row_w ? (W - row_w) / 2u : 0u;
    const std::string pad(margin, ' ');
    const std::string sep(row_w, '-');

    std::println("{}", title_line);
    std::println("{}{}  {}  {}  {}", pad, str_ljust(h_algo, w_algo), str_center(h_ans, w_ans), str_center(h_time, w_time), str_center(h_rel, w_rel)); 
    std::println("{}{}", pad, sep);
    std::println("{}{}  {}  {}  {}",pad, str_ljust(algo_seq, w_algo), str_center(ans_seq, w_ans), str_center(t_seq, w_time), str_center(rel_seq, w_rel));
    std::println("{}{}  {}  {}  {}", pad, str_ljust(algo_par, w_algo), str_center(ans_par, w_ans), str_center(t_par, w_time), str_center(rel_par, w_rel));
    std::println("{}{}", pad, sep);
    std::println("{}", make_border(W));

    return (seq_result || par_result) ? 0 : 1;
}



static void run_benchmark() {

    static constexpr struct { uint64_t p; uint64_t alpha; } problems[] = {
        {997u,   7u },
        {7919u,   7u },
        {99991u,   6u },
        {999983u,   5u },
        {9999991u,  22u },
        {99999989u,   2u },
        {999999937u,  11u },
        {9999999967u,  10u },
        {99999999977u,   3u },
        {999999999989u,   2u },
    };

    const std::string h_p = "p (порядок)";
    const std::string h_seq = "Послідовний (мс)";
    const std::string h_par = "Паралельний (мс)";
    const std::string h_speedup = "Прискорення";

    const std::size_t w_p = std::max(utf8_width(h_p), std::size_t{ 14u });
    const std::size_t w_s = std::max(utf8_width(h_seq), std::size_t{ 18u });
    const std::size_t w_pr = std::max(utf8_width(h_par), std::size_t{ 18u });
    const std::size_t w_sp = std::max(utf8_width(h_speedup), std::size_t{ 11u });

    const std::size_t row_w = w_p + w_s + w_pr + w_sp + 9u;
    auto [title_line, W] = make_section_title("Benchmark: Index-calculus seq vs par", row_w + 4u);

    const std::size_t margin = W > row_w ? (W - row_w) / 2u : 0u;
    const std::string pad(margin, ' ');
    const std::string sep(row_w, '-');

    std::println("{}", title_line);
    std::println("{}{}  {}  {}  {}", pad, str_ljust(h_p, w_p), str_center(h_seq, w_s), str_center(h_par, w_pr), str_center(h_speedup, w_sp));
    std::println("{}{}", pad, sep);

    constexpr int measurements = 5;
    constexpr double timeout_ms = 300'000.0;

    for (const auto& [p, alpha] : problems) {
        const uint64_t beta = modpow(alpha, uint64_t{ 3u }, p);

        double sum_seq = 0.0, sum_par = 0.0;
        int ok_seq = 0, ok_par = 0;
        bool timed_out = false;

        for (int m = 0; m < measurements; ++m) {
            const auto rs = index_calculus(p, alpha, beta);
            const auto rp = index_calculus_par(p, alpha, beta);

            if (rs) { sum_seq += rs->elapsed_ms; ++ok_seq; }
            if (rp) { sum_par += rp->elapsed_ms; ++ok_par; }

            const double cur_seq = ok_seq > 0 ? sum_seq / ok_seq : 0.0;
            const double cur_par = ok_par > 0 ? sum_par / ok_par : 0.0;
            if (cur_seq > timeout_ms || cur_par > timeout_ms) {
                timed_out = true;
                break;
            }
        }

        const double avg_seq = ok_seq > 0 ? sum_seq / ok_seq : -1.0;
        const double avg_par = ok_par > 0 ? sum_par / ok_par : -1.0;
        const double speedup = (avg_par > 0.0 && avg_seq > 0.0) ? avg_seq / avg_par : 0.0;

        char s_seq[24], s_par[24], s_sp[16];
        if (timed_out) {
            std::snprintf(s_seq, sizeof(s_seq), "тайм-аут");
            std::snprintf(s_par, sizeof(s_par), "тайм-аут");
            std::snprintf(s_sp, sizeof(s_sp), "-");
        }
        else if (avg_seq < 0.0 || avg_par < 0.0) {
            std::snprintf(s_seq, sizeof(s_seq), avg_seq < 0.0 ? "помилка" : "%.2f", avg_seq);
            std::snprintf(s_par, sizeof(s_par), avg_par < 0.0 ? "помилка" : "%.2f", avg_par);
            std::snprintf(s_sp, sizeof(s_sp), "-");
        }
        else {
            std::snprintf(s_seq, sizeof(s_seq), "%.2f", avg_seq);
            std::snprintf(s_par, sizeof(s_par), "%.2f", avg_par);
            std::snprintf(s_sp, sizeof(s_sp), "%.2fx", speedup);
        }

        std::println("{}{}  {}  {}  {}", pad, str_ljust(std::to_string(p), w_p), str_center(s_seq, w_s), str_center(s_par, w_pr), str_center(s_sp, w_sp));

        if (timed_out) break;
    }

    std::println("{}{}", pad, sep);
    std::println("{}", make_border(W));
}

static void print_help(const std::string& prog) {

    const std::vector<std::string> lines = {
        "Використання:",
        "  " + prog + " --seq   p alpha beta",
        "  " + prog + " --par   p alpha beta [потоків]",
        "  " + prog + " --both  p alpha beta [потоків]",
        "  " + prog + " --benchmark",
        "",
        "РЕЖИМИ:",
        "  --seq           Послідовний index-calculus",
        "  --par           Паралельний index-calculus (розпаралелення збору)",
        "  --both          Обидва алгоритми з таблицею порівняння",
        "  -b, --benchmark Автоматичний тест із зростанням розміру p",
        "  -h, --help      Ця довідка",
        "",
        "АРГУМЕНТИ:",
        "  p               Просте число (модуль)",
        "  alpha           Первісний корінь по модулю p",
        "  beta            Елемент групи (1 ≤ beta ≤ p-1)",
        "  потоків         Кількість потоків для --par / --both (0 = auto)",
        "",
        "ФОРМАТИ ЧИСЛА:",
        "  десятковий  : 999983",
        "  шістнадцятк : 0xF423F",
        "  двійковий   : 0b11110100001000111111",
        "",
        "ПРИКЛАДИ:",
        "  " + prog + " --seq  999983 2 12345",
        "  " + prog + " --par  999983 2 12345 4",
        "  " + prog + " --both 9999991 2 987654",
        "  " + prog + " --benchmark",
    };

    std::size_t cw = 0u;
    for (const auto& l : lines) cw = std::max(cw, utf8_width(l));
    auto [title_line, TW] = make_section_title("Довідка", cw + 4u);
    const std::size_t cw_box = TW > 4u ? TW - 4u : cw;

    std::println("{}", title_line);
    for (const auto& l : lines) std::println("• {} •", str_ljust(l, cw_box));
    std::println("{}", make_border(TW));
}


struct Args {
    enum class Mode : uint8_t { Seq, Par, Both, Benchmark, Help };

    Mode mode = Mode::Help;
    uint64_t p = 0u;
    uint64_t alpha = 0u;
    uint64_t beta = 0u;
    unsigned threads = 0u;
    bool error = false;
    std::string error_msg;
};

static Args parse_args(int argc, char** argv) noexcept {
    Args a;
    if (argc < 2) return a; 

    const std::string mode_flag = argv[1];
    if (mode_flag == "-h" || mode_flag == "--help") {
        a.mode = Args::Mode::Help;
        return a;
    }
    if (mode_flag == "-b" || mode_flag == "--benchmark") {
        a.mode = Args::Mode::Benchmark;
        return a;
    }

    if (mode_flag == "--seq")  a.mode = Args::Mode::Seq;
    else if (mode_flag == "--par")  a.mode = Args::Mode::Par;
    else if (mode_flag == "--both") a.mode = Args::Mode::Both;
    else {
        a.error = true;
        a.error_msg = "Невідомий режим: " + mode_flag;
        return a;
    }

    if (argc < 5) {
        a.error = true;
        a.error_msg = "Очікується: " + mode_flag + " p alpha beta [потоків]";
        return a;
    }

    const auto parse = [](const char* s) -> std::optional<uint64_t> {
        auto r = parse_number(s);
        if (!r) return std::nullopt;
        return *r;
        };

    const auto mp = parse(argv[2]);
    const auto ma = parse(argv[3]);
    const auto mb = parse(argv[4]);

    if (!mp || !ma || !mb) {
        a.error = true;
        a.error_msg = "Некоректний формат числа в аргументах.";
        return a;
    }

    a.p = *mp; a.alpha = *ma; a.beta = *mb;

    if (argc >= 6) {
        const auto mt = parse(argv[5]);
        if (!mt) { a.error = true; a.error_msg = "Некоректна кількість потоків."; return a; }
        a.threads = static_cast<unsigned>(*mt);
    }

    return a;
}


int main(int argc, char** argv) {
    const std::string prog = (argc > 0) ? argv[0] : "./Lab3";
    const Args args = parse_args(argc, argv);

    if (args.error) {
        std::println(stderr, "Помилка: {}", args.error_msg);
        std::println(stderr, "Запустіть {} --help для довідки.", prog);
        return 1;
    }

    switch (args.mode) {
    case Args::Mode::Help:      print_help(prog);  return 0;
    case Args::Mode::Benchmark: run_benchmark();   return 0;
    case Args::Mode::Seq:       return run_seq(args.p, args.alpha, args.beta);
    case Args::Mode::Par:       return run_par(args.p, args.alpha, args.beta, args.threads);
    case Args::Mode::Both:      return run_both(args.p, args.alpha, args.beta, args.threads);
    }
    return 0;
}