#include "pgnstats/stats.hpp"

#include <algorithm>
#include <cctype>

namespace pgnstats {
namespace {

std::string lowercase(std::string_view s) {
    std::string out(s);
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

}  // namespace

double Record::score_pct() const {
    const unsigned finished = wins + draws + losses;
    if (finished == 0) return 0.0;
    return 100.0 * (wins + 0.5 * draws) / finished;
}

double Stats::average_plies() const {
    return games_ == 0 ? 0.0 : static_cast<double>(plies_) / games_;
}

void Stats::add(const Game& game) {
    ++games_;
    plies_ += game.moves.size();
    switch (game.result) {
        case Result::WhiteWin: ++white_wins_; break;
        case Result::BlackWin: ++black_wins_; break;
        case Result::Draw:     ++draws_;      break;
        case Result::Unknown:  ++unfinished_; break;
    }

    // Prefer the file's own classification; fall back to the trie.
    std::string eco = game.tag_or("ECO", "");
    std::string name = game.tag_or("Opening", "");
    if (!eco.empty()) {
        ++eco_from_tags_;
    } else if (const Opening* detected = openings_.longest_match(game.moves)) {
        ++eco_detected_;
        eco = detected->eco;
        if (name.empty()) name = detected->name;
    } else {
        eco = "?";
        name = "Unclassified";
    }
    if (name.empty()) {
        if (const Opening* detected = openings_.longest_match(game.moves)) name = detected->name;
    }

    OpeningCount& opening = by_eco_[eco];
    if (opening.eco.empty()) opening.eco = eco;
    if (opening.name.empty()) opening.name = name;
    ++opening.games;
    if (game.result == Result::WhiteWin) ++opening.white_wins;
    if (game.result == Result::Draw) ++opening.draws;
    if (game.result == Result::BlackWin) ++opening.black_wins;

    const std::string white = game.tag_or("White", "Unknown");
    const std::string black = game.tag_or("Black", "Unknown");
    credit(white, black, true, game);
    credit(black, white, false, game);
}

void Stats::credit(const std::string& name, const std::string& opponent, bool white,
                   const Game& game) {
    PlayerSummary& player = players_[name];
    if (player.name.empty()) player.name = name;

    Record& r = player.record;
    (white ? r.as_white : r.as_black)++;
    switch (game.result) {
        case Result::WhiteWin: (white ? r.wins : r.losses)++; break;
        case Result::BlackWin: (white ? r.losses : r.wins)++; break;
        case Result::Draw:     ++r.draws; break;
        case Result::Unknown:  ++r.unfinished; break;
    }

    player.games.push_back({opponent, white, game.result, game.tag_or("Event", "?"),
                            game.tag_or("Date", "????.??.??")});
}

std::vector<OpeningCount> Stats::top_openings(std::size_t n) const {
    std::vector<OpeningCount> sorted;
    sorted.reserve(by_eco_.size());
    for (const auto& entry : by_eco_) sorted.push_back(entry.second);
    std::sort(sorted.begin(), sorted.end(), [](const OpeningCount& a, const OpeningCount& b) {
        if (a.games != b.games) return a.games > b.games;
        // Among ties, unclassified ("?") goes after real ECO codes.
        if ((a.eco == "?") != (b.eco == "?")) return b.eco == "?";
        return a.eco < b.eco;
    });
    if (sorted.size() > n) sorted.resize(n);
    return sorted;
}

std::vector<PlayerSummary> Stats::top_players(std::size_t n) const {
    std::vector<PlayerSummary> sorted;
    sorted.reserve(players_.size());
    for (const auto& entry : players_) sorted.push_back(entry.second);
    std::sort(sorted.begin(), sorted.end(), [](const PlayerSummary& a, const PlayerSummary& b) {
        if (a.record.games() != b.record.games()) return a.record.games() > b.record.games();
        if (a.record.score_pct() != b.record.score_pct())
            return a.record.score_pct() > b.record.score_pct();
        return a.name < b.name;
    });
    if (sorted.size() > n) sorted.resize(n);
    return sorted;
}

std::vector<const PlayerSummary*> Stats::find_player(std::string_view query) const {
    const std::string needle = lowercase(query);
    std::vector<const PlayerSummary*> partial;
    for (const auto& entry : players_) {
        const std::string hay = lowercase(entry.first);
        if (hay == needle) return {&entry.second};
        if (hay.find(needle) != std::string::npos) partial.push_back(&entry.second);
    }
    std::sort(partial.begin(), partial.end(),
              [](const PlayerSummary* a, const PlayerSummary* b) { return a->name < b->name; });
    return partial;
}

}  // namespace pgnstats
