#include "pgnstats/report.hpp"

#include <iomanip>
#include <sstream>

namespace pgnstats {
namespace {

constexpr int kBarWidth = 30;

double pct(unsigned part, unsigned whole) {
    return whole == 0 ? 0.0 : 100.0 * part / whole;
}

std::string fixed1(double value) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(1) << value;
    return s.str();
}

std::string bar(double percent) {
    const int filled = static_cast<int>(percent / 100.0 * kBarWidth + 0.5);
    return std::string(static_cast<std::size_t>(filled), '#') +
           std::string(static_cast<std::size_t>(kBarWidth - filled), '.');
}

std::string wdl(unsigned w, unsigned d, unsigned l) {
    return std::to_string(w) + "/" + std::to_string(d) + "/" + std::to_string(l);
}

std::string truncate(const std::string& s, std::size_t width) {
    return s.size() <= width ? s : s.substr(0, width - 3) + "...";
}

void heading(std::ostream& out, const std::string& title) {
    out << '\n' << title << '\n' << std::string(title.size(), '-') << '\n';
}

}  // namespace

void print_summary(std::ostream& out, const std::string& source, const Stats& stats,
                   std::size_t top) {
    const unsigned n = stats.games();
    out << "pgn-stats: " << source << '\n';
    out << "Games: " << n << "   average length: " << fixed1(stats.average_plies())
        << " plies\n";

    heading(out, "Results");
    out << "  1-0      " << std::setw(5) << stats.white_wins() << '\n';
    out << "  0-1      " << std::setw(5) << stats.black_wins() << '\n';
    out << "  1/2-1/2  " << std::setw(5) << stats.draws() << '\n';
    if (stats.unfinished() > 0)
        out << "  *        " << std::setw(5) << stats.unfinished() << '\n';

    heading(out, "Win rate by color");
    const double white = pct(stats.white_wins(), n);
    const double draw = pct(stats.draws(), n);
    const double black = pct(stats.black_wins(), n);
    out << "  White wins  " << bar(white) << ' ' << std::setw(5) << fixed1(white) << "%\n";
    out << "  Draws       " << bar(draw) << ' ' << std::setw(5) << fixed1(draw) << "%\n";
    out << "  Black wins  " << bar(black) << ' ' << std::setw(5) << fixed1(black) << "%\n";
    const unsigned finished = stats.white_wins() + stats.draws() + stats.black_wins();
    if (finished > 0) {
        const double white_score = 100.0 * (stats.white_wins() + 0.5 * stats.draws()) / finished;
        out << "  White scores " << fixed1(white_score) << "% of the points\n";
    }

    heading(out, "Top openings (ECO)");
    out << "  ECO   Games  W/D/L     Opening\n";
    for (const OpeningCount& o : stats.top_openings(top)) {
        out << "  " << std::left << std::setw(5) << o.eco << std::right << std::setw(6)
            << o.games << "  " << std::left << std::setw(8)
            << wdl(o.white_wins, o.draws, o.black_wins) << "  " << truncate(o.name, 48)
            << std::right << '\n';
    }
    out << "  (" << stats.eco_from_tags() << " from ECO tags, " << stats.eco_detected()
        << " detected from moves)\n";

    heading(out, "Top players");
    out << "  Player                      Games    +    =    -   Score\n";
    for (const PlayerSummary& p : stats.top_players(top)) {
        const Record& r = p.record;
        out << "  " << std::left << std::setw(26) << truncate(p.name, 26) << std::right
            << std::setw(7) << r.games() << std::setw(5) << r.wins << std::setw(5) << r.draws
            << std::setw(5) << r.losses << std::setw(7) << fixed1(r.score_pct()) << "%\n";
    }
}

void print_player(std::ostream& out, const PlayerSummary& player) {
    const Record& r = player.record;
    out << player.name << '\n' << std::string(player.name.size(), '=') << '\n';
    out << "Games: " << r.games() << " (" << r.as_white << " as White, " << r.as_black
        << " as Black)\n";
    out << "Record: +" << r.wins << " =" << r.draws << " -" << r.losses;
    if (r.unfinished > 0) out << " (" << r.unfinished << " unfinished)";
    out << "   Score: " << fixed1(r.score_pct()) << "%\n";

    heading(out, "Games");
    for (const PlayerGame& g : player.games) {
        out << "  " << g.date << "  " << (g.white ? "W" : "B") << "  " << std::left
            << std::setw(8) << to_string(g.result) << std::setw(28) << truncate(g.opponent, 26)
            << truncate(g.event, 30) << std::right << '\n';
    }
}

}  // namespace pgnstats
