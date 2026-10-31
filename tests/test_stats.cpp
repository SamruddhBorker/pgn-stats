#include <fstream>
#include <sstream>
#include <string>

#include "doctest/doctest.h"
#include "pgnstats/parser.hpp"
#include "pgnstats/report.hpp"
#include "pgnstats/stats.hpp"

using pgnstats::OpeningTrie;
using pgnstats::PgnParser;
using pgnstats::Stats;

namespace {

void feed(Stats& stats, std::istream& in) {
    PgnParser parser(in);
    while (auto game = parser.next()) stats.add(*game);
}

void feed(Stats& stats, const std::string& text) {
    std::istringstream in(text);
    feed(stats, in);
}

constexpr const char* kSmall = R"(
[White "Alice"] [Black "Bob"] [ECO "C60"] [Opening "Ruy Lopez"]
1. e4 e5 2. Nf3 Nc6 3. Bb5 1-0

[White "Bob"] [Black "Alice"]
1. d4 d5 2. c4 e6 1/2-1/2

[White "Carol"] [Black "Alice"]
1. e4 e5 2. Nf3 Nc6 3. Bb5 0-1

[White "Bob"] [Black "Carol"]
1. g4 *
)";

}  // namespace

TEST_CASE("record score") {
    pgnstats::Record r;
    CHECK(r.score_pct() == doctest::Approx(0.0));
    r.wins = 1;
    r.draws = 1;
    r.losses = 2;
    r.unfinished = 5;
    CHECK(r.games() == 9);
    CHECK(r.score_pct() == doctest::Approx(37.5));
}

TEST_CASE("result totals and average length") {
    const OpeningTrie trie = OpeningTrie::with_builtin_openings();
    Stats stats(trie);
    feed(stats, kSmall);
    CHECK(stats.games() == 4);
    CHECK(stats.white_wins() == 1);
    CHECK(stats.black_wins() == 1);
    CHECK(stats.draws() == 1);
    CHECK(stats.unfinished() == 1);
    CHECK(stats.average_plies() == doctest::Approx((5 + 4 + 5 + 1) / 4.0));
}

TEST_CASE("openings use ECO tags first, then the trie") {
    const OpeningTrie trie = OpeningTrie::with_builtin_openings();
    Stats stats(trie);
    feed(stats, kSmall);
    CHECK(stats.eco_from_tags() == 1);
    CHECK(stats.eco_detected() == 2);  // 1.d4 line and the second Ruy Lopez

    const auto top = stats.top_openings(10);
    REQUIRE(top.size() == 3);
    CHECK(top[0].eco == "C60");
    CHECK(top[0].games == 2);
    CHECK(top[0].white_wins == 1);
    CHECK(top[0].black_wins == 1);
    CHECK(top[1].eco == "D30");  // ties broken by ECO code
    CHECK(top[1].name == "Queen's Gambit Declined");
    CHECK(top[2].eco == "?");    // 1.g4 is not in the table

    CHECK(stats.top_openings(1).size() == 1);
}

TEST_CASE("per-player records") {
    const OpeningTrie trie = OpeningTrie::with_builtin_openings();
    Stats stats(trie);
    feed(stats, kSmall);

    const auto players = stats.top_players(10);
    REQUIRE(players.size() == 3);
    // Alice and Bob both have 3 games; Alice scores 2/3 vs Bob's 0.5/2 finished.
    CHECK(players[0].name == "Alice");
    CHECK(players[0].record.wins == 2);
    CHECK(players[0].record.draws == 1);
    CHECK(players[0].record.losses == 0);
    CHECK(players[0].record.as_black == 2);
    CHECK(players[1].name == "Bob");
    CHECK(players[1].record.unfinished == 1);
    CHECK(players[2].name == "Carol");
    CHECK(players[2].record.losses == 1);
}

TEST_CASE("find_player matching") {
    const OpeningTrie trie = OpeningTrie::with_builtin_openings();
    Stats stats(trie);
    feed(stats, kSmall);

    auto m = stats.find_player("alice");
    REQUIRE(m.size() == 1);
    CHECK(m[0]->name == "Alice");
    CHECK(m[0]->games.size() == 3);
    CHECK(m[0]->games[0].opponent == "Bob");
    CHECK(m[0]->games[0].white);

    CHECK(stats.find_player("car").size() == 1);
    CHECK(stats.find_player("o").size() == 2);  // Bob, Carol
    CHECK(stats.find_player("zed").empty());
}

TEST_CASE("bundled sample database") {
    std::ifstream in(PGNSTATS_DATA_DIR "/classics.pgn", std::ios::binary);
    REQUIRE(in);
    const OpeningTrie trie = OpeningTrie::with_builtin_openings();
    Stats stats(trie);
    feed(stats, in);

    CHECK(stats.games() == 12);
    CHECK(stats.white_wins() == 7);
    CHECK(stats.black_wins() == 4);
    CHECK(stats.draws() == 1);
    CHECK(stats.eco_detected() == 2);

    const auto top = stats.top_openings(1);
    REQUIRE(top.size() == 1);
    CHECK(top[0].eco == "C41");
    CHECK(top[0].games == 3);

    const auto morphy = stats.find_player("morphy");
    REQUIRE(morphy.size() == 1);
    CHECK(morphy[0]->record.wins == 3);
    CHECK(morphy[0]->record.as_black == 2);

    std::ostringstream report;
    pgnstats::print_summary(report, "classics.pgn", stats, 5);
    CHECK(report.str().find("Games: 12") != std::string::npos);
    CHECK(report.str().find("Anderssen, Adolf") != std::string::npos);
}
