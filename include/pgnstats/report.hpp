#pragma once

#include <cstddef>
#include <ostream>
#include <string>

#include "pgnstats/stats.hpp"

namespace pgnstats {

void print_summary(std::ostream& out, const std::string& source, const Stats& stats,
                   std::size_t top);

void print_player(std::ostream& out, const PlayerSummary& player);

}  // namespace pgnstats
