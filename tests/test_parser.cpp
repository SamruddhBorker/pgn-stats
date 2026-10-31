#include <sstream>
#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "pgnstats/parser.hpp"

using pgnstats::Game;
using pgnstats::PgnParser;
using pgnstats::Result;

namespace {

std::vector<Game> parse_all(const std::string& text) {
    std::istringstream in(text);
    PgnParser parser(in);
    std::vector<Game> games;
    while (auto game = parser.next()) games.push_back(std::move(*game));
    return games;
}

}  // namespace

TEST_CASE("normalize_san") {
    CHECK(pgnstats::normalize_san("Qxh7+") == "Qxh7");
    CHECK(pgnstats::normalize_san("Rd8#") == "Rd8");
    CHECK(pgnstats::normalize_san("Nxb5!?") == "Nxb5");
    CHECK(pgnstats::normalize_san("0-0") == "O-O");
    CHECK(pgnstats::normalize_san("0-0-0+") == "O-O-O");
    CHECK(pgnstats::normalize_san("e8=Q") == "e8=Q");
}

TEST_CASE("parse_result round trip") {
    for (Result r : {Result::WhiteWin, Result::BlackWin, Result::Draw, Result::Unknown})
        CHECK(pgnstats::parse_result(pgnstats::to_string(r)) == r);
    CHECK(pgnstats::parse_result("garbage") == Result::Unknown);
}

TEST_CASE("single game with tags and moves") {
    const auto games = parse_all(R"(
[Event "Test"]
[White "A"]
[Black "B"]
[Result "1-0"]

1. e4 e5 2. Qh5 Nc6 3. Bc4 Nf6 4. Qxf7# 1-0
)");
    REQUIRE(games.size() == 1);
    const Game& g = games[0];
    CHECK(g.tags.size() == 4);
    CHECK(*g.tag("White") == "A");
    CHECK(g.tag("Missing") == nullptr);
    CHECK(g.result == Result::WhiteWin);
    CHECK(g.moves == std::vector<std::string>{"e4", "e5", "Qh5", "Nc6", "Bc4", "Nf6", "Qxf7"});
}

TEST_CASE("comments, NAGs and variations stay out of the main line") {
    const auto games = parse_all(
        "1. e4 {comment} e5 $1 2. Nf3 (2. f4 exf4 (2... d5) 3. Nf3) 2... Nc6!? ; note\n"
        "3. Bb5 *");
    REQUIRE(games.size() == 1);
    const Game& g = games[0];
    CHECK(g.moves == std::vector<std::string>{"e4", "e5", "Nf3", "Nc6", "Bb5"});
    CHECK(g.comments == 2);
    CHECK(g.nags == 2);
    CHECK(g.variations == 2);
    CHECK(g.result == Result::Unknown);
}

TEST_CASE("a result inside a variation does not end the game") {
    const auto games = parse_all("1. e4 (1. f3 e5 2. g4 Qh4# 0-1) 1... e5 1/2-1/2");
    REQUIRE(games.size() == 1);
    CHECK(games[0].moves == std::vector<std::string>{"e4", "e5"});
    CHECK(games[0].result == Result::Draw);
}

TEST_CASE("multiple games are split on result tokens") {
    const auto games = parse_all(R"(
[White "A"] 1. e4 e5 1-0
[White "B"] 1. d4 d5 0-1
[White "C"] 1. c4 1/2-1/2
)");
    REQUIRE(games.size() == 3);
    CHECK(*games[1].tag("White") == "B");
    CHECK(games[1].result == Result::BlackWin);
    CHECK(games[2].moves.size() == 1);
}

TEST_CASE("missing movetext result falls back to the Result tag") {
    const auto games = parse_all(R"(
[Result "0-1"]
1. f3 e5 2. g4 Qh4

[Result "1-0"]
1. e4
)");
    REQUIRE(games.size() == 2);
    CHECK(games[0].result == Result::BlackWin);
    CHECK(games[0].moves.size() == 4);
    CHECK(games[1].result == Result::WhiteWin);
}

TEST_CASE("malformed tags are skipped") {
    const auto games = parse_all("[Broken] [Event \"ok\"] [ \"no name\"] 1. e4 *");
    REQUIRE(games.size() == 1);
    REQUIRE(games[0].tags.size() == 1);
    CHECK(games[0].tags[0].first == "Event");
}

TEST_CASE("empty input yields no games") {
    CHECK(parse_all("").empty());
    CHECK(parse_all("   \n% only an escape line\n").empty());
}

TEST_CASE("tag_or ignores placeholder values") {
    const auto games = parse_all("[Event \"?\"] [Site \"\"] [Round \"3\"] *");
    REQUIRE(games.size() == 1);
    CHECK(games[0].tag_or("Event", "none") == "none");
    CHECK(games[0].tag_or("Site", "none") == "none");
    CHECK(games[0].tag_or("Round", "none") == "3");
}
