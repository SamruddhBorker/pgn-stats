#include "pgnstats/opening_trie.hpp"

#include <sstream>

namespace pgnstats {

OpeningTrie::OpeningTrie() : nodes_(1) {}

void OpeningTrie::insert(const std::string& line, Opening opening) {
    std::istringstream moves(line);
    std::size_t node = 0;
    for (std::string move; moves >> move;) {
        auto it = nodes_[node].children.find(move);
        if (it == nodes_[node].children.end()) {
            // Grab the index before push_back: it may reallocate nodes_.
            const std::size_t child = nodes_.size();
            nodes_[node].children.emplace(move, child);
            nodes_.emplace_back();
            node = child;
        } else {
            node = it->second;
        }
    }
    if (!nodes_[node].opening) ++openings_;
    nodes_[node].opening = std::move(opening);
}

const Opening* OpeningTrie::longest_match(const std::vector<std::string>& moves,
                                          std::size_t* depth) const {
    const Opening* best = nullptr;
    std::size_t best_depth = 0;
    std::size_t node = 0;
    for (std::size_t ply = 0; ply < moves.size(); ++ply) {
        auto it = nodes_[node].children.find(moves[ply]);
        if (it == nodes_[node].children.end()) break;
        node = it->second;
        if (nodes_[node].opening) {
            best = &*nodes_[node].opening;
            best_depth = ply + 1;
        }
    }
    if (depth) *depth = best_depth;
    return best;
}

OpeningTrie OpeningTrie::with_builtin_openings() {
    struct Entry {
        const char* line;
        const char* eco;
        const char* name;
    };
    static const Entry kOpenings[] = {
        {"e4", "B00", "King's Pawn Opening"},
        {"e4 e5", "C20", "King's Pawn Game"},
        {"e4 e5 Bc4", "C23", "Bishop's Opening"},
        {"e4 e5 Nc3", "C25", "Vienna Game"},
        {"e4 e5 f4", "C30", "King's Gambit"},
        {"e4 e5 f4 exf4", "C33", "King's Gambit Accepted"},
        {"e4 e5 f4 exf4 Bc4", "C33", "King's Gambit Accepted, Bishop's Gambit"},
        {"e4 e5 Nf3", "C40", "King's Knight Opening"},
        {"e4 e5 Nf3 d6", "C41", "Philidor Defense"},
        {"e4 e5 Nf3 Nf6", "C42", "Petrov's Defense"},
        {"e4 e5 Nf3 Nc6", "C44", "King's Pawn Game"},
        {"e4 e5 Nf3 Nc6 d4", "C44", "Scotch Game"},
        {"e4 e5 Nf3 Nc6 Nc3", "C46", "Three Knights Game"},
        {"e4 e5 Nf3 Nc6 Nc3 Nf6", "C47", "Four Knights Game"},
        {"e4 e5 Nf3 Nc6 Nc3 Nf6 Bb5", "C48", "Four Knights Game, Spanish Variation"},
        {"e4 e5 Nf3 Nc6 Bc4", "C50", "Italian Game"},
        {"e4 e5 Nf3 Nc6 Bc4 Bc5", "C50", "Giuoco Piano"},
        {"e4 e5 Nf3 Nc6 Bc4 Bc5 b4", "C51", "Evans Gambit"},
        {"e4 e5 Nf3 Nc6 Bc4 Nf6", "C55", "Two Knights Defense"},
        {"e4 e5 Nf3 Nc6 Bb5", "C60", "Ruy Lopez"},
        {"e4 e5 Nf3 Nc6 Bb5 a6", "C68", "Ruy Lopez, Morphy Defense"},
        {"e4 c5", "B20", "Sicilian Defense"},
        {"e4 c5 Nf3 d6 d4 cxd4 Nxd4 Nf6 Nc3 a6", "B90", "Sicilian Defense, Najdorf Variation"},
        {"e4 e6", "C00", "French Defense"},
        {"e4 c6", "B10", "Caro-Kann Defense"},
        {"e4 c6 d4 d5 Nc3", "B15", "Caro-Kann Defense"},
        {"e4 d5", "B01", "Scandinavian Defense"},
        {"e4 d6", "B07", "Pirc Defense"},
        {"d4", "A40", "Queen's Pawn Game"},
        {"d4 d5", "D00", "Queen's Pawn Game"},
        {"d4 d5 c4", "D06", "Queen's Gambit"},
        {"d4 d5 c4 dxc4", "D20", "Queen's Gambit Accepted"},
        {"d4 d5 c4 e6", "D30", "Queen's Gambit Declined"},
        {"d4 d5 c4 c6", "D10", "Slav Defense"},
        {"d4 Nf6", "A45", "Indian Defense"},
        {"d4 Nf6 c4", "A50", "Indian Defense"},
        {"d4 Nf6 c4 g6", "E60", "King's Indian Defense"},
        {"d4 Nf6 c4 e6 Nc3 Bb4", "E20", "Nimzo-Indian Defense"},
        {"d4 f5", "A80", "Dutch Defense"},
        {"c4", "A10", "English Opening"},
        {"Nf3", "A04", "Reti Opening"},
    };

    OpeningTrie trie;
    for (const Entry& e : kOpenings) trie.insert(e.line, {e.eco, e.name});
    return trie;
}

}  // namespace pgnstats
