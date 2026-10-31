#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "pgnstats/opening_trie.hpp"
#include "pgnstats/parser.hpp"

namespace pgnstats {

struct Record {
    unsigned wins = 0;
    unsigned draws = 0;
    unsigned losses = 0;
    unsigned unfinished = 0;
    unsigned as_white = 0;
    unsigned as_black = 0;

    unsigned games() const { return wins + draws + losses + unfinished; }
    // Points scored over finished games, as a percentage (0 if none).
    double score_pct() const;
};

struct PlayerGame {
    std::string opponent;
    bool white = true;
    Result result = Result::Unknown;
    std::string event;
    std::string date;
};

struct PlayerSummary {
    std::string name;
    Record record;
    std::vector<PlayerGame> games;
};

struct OpeningCount {
    std::string eco;
    std::string name;
    unsigned games = 0;
    unsigned white_wins = 0;
    unsigned draws = 0;
    unsigned black_wins = 0;
};

class Stats {
public:
    explicit Stats(const OpeningTrie& openings) : openings_(openings) {}

    void add(const Game& game);

    unsigned games() const { return games_; }
    unsigned white_wins() const { return white_wins_; }
    unsigned black_wins() const { return black_wins_; }
    unsigned draws() const { return draws_; }
    unsigned unfinished() const { return unfinished_; }
    unsigned eco_from_tags() const { return eco_from_tags_; }
    unsigned eco_detected() const { return eco_detected_; }
    double average_plies() const;

    // Most played openings, by game count then ECO code.
    std::vector<OpeningCount> top_openings(std::size_t n) const;
    // Most active players, by game count then score then name.
    std::vector<PlayerSummary> top_players(std::size_t n) const;

    // Case-insensitive exact name match, falling back to a unique
    // substring match. Returns every candidate when ambiguous.
    std::vector<const PlayerSummary*> find_player(std::string_view query) const;

private:
    void credit(const std::string& name, const std::string& opponent, bool white,
                const Game& game);

    const OpeningTrie& openings_;
    std::unordered_map<std::string, PlayerSummary> players_;
    std::unordered_map<std::string, OpeningCount> by_eco_;
    unsigned games_ = 0;
    unsigned white_wins_ = 0;
    unsigned black_wins_ = 0;
    unsigned draws_ = 0;
    unsigned unfinished_ = 0;
    unsigned eco_from_tags_ = 0;
    unsigned eco_detected_ = 0;
    unsigned long long plies_ = 0;
};

}  // namespace pgnstats
