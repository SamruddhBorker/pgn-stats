#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#include "pgnstats/opening_trie.hpp"
#include "pgnstats/parser.hpp"
#include "pgnstats/report.hpp"
#include "pgnstats/stats.hpp"

namespace {

constexpr const char* kUsage =
    "usage: pgn-stats <games.pgn | -> [--top N] [--player NAME]\n"
    "\n"
    "  --top N        rows to show in the openings and players tables (default 10)\n"
    "  --player NAME  show one player's record and game list instead\n"
    "  -h, --help     show this help\n";

struct Options {
    std::string path;
    std::size_t top = 10;
    std::optional<std::string> player;
};

std::optional<Options> parse_args(int argc, char** argv) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << kUsage;
            std::exit(EXIT_SUCCESS);
        } else if (arg == "--top" && i + 1 < argc) {
            try {
                const long n = std::stol(argv[++i]);
                if (n <= 0) return std::nullopt;
                opts.top = static_cast<std::size_t>(n);
            } catch (const std::exception&) {
                return std::nullopt;
            }
        } else if (arg == "--player" && i + 1 < argc) {
            opts.player = argv[++i];
        } else if (opts.path.empty() && (arg == "-" || arg.rfind("--", 0) != 0)) {
            opts.path = arg;
        } else {
            return std::nullopt;
        }
    }
    if (opts.path.empty()) return std::nullopt;
    return opts;
}

}  // namespace

int main(int argc, char** argv) {
    const std::optional<Options> opts = parse_args(argc, argv);
    if (!opts) {
        std::cerr << kUsage;
        return 2;
    }

    std::ifstream file;
    if (opts->path != "-") {
        file.open(opts->path, std::ios::binary);
        if (!file) {
            std::cerr << "pgn-stats: cannot open '" << opts->path << "'\n";
            return 1;
        }
    }
    std::istream& in = opts->path == "-" ? std::cin : file;

    const pgnstats::OpeningTrie openings = pgnstats::OpeningTrie::with_builtin_openings();
    pgnstats::Stats stats(openings);
    pgnstats::PgnParser parser(in);
    while (std::optional<pgnstats::Game> game = parser.next()) stats.add(*game);

    if (opts->player) {
        const auto matches = stats.find_player(*opts->player);
        if (matches.empty()) {
            std::cerr << "pgn-stats: no player matching '" << *opts->player << "'\n";
            return 1;
        }
        if (matches.size() > 1) {
            std::cerr << "pgn-stats: '" << *opts->player << "' is ambiguous:\n";
            for (const auto* p : matches) std::cerr << "  " << p->name << '\n';
            return 1;
        }
        pgnstats::print_player(std::cout, *matches.front());
        return 0;
    }

    pgnstats::print_summary(std::cout, opts->path == "-" ? "<stdin>" : opts->path, stats,
                            opts->top);
    return 0;
}
