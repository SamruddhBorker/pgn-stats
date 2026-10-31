#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace pgnstats {

struct Opening {
    std::string eco;   // e.g. "C41"
    std::string name;  // e.g. "Philidor Defense"
};

// A trie keyed by SAN moves. Each node may carry the opening that its
// move sequence defines; lookups return the deepest such node along a
// game's main line, i.e. the most specific opening it reached.
class OpeningTrie {
public:
    OpeningTrie();

    // `line` is a space-separated SAN sequence such as "e4 e5 Nf3 d6".
    void insert(const std::string& line, Opening opening);

    // Deepest opening whose line is a prefix of `moves`, or nullptr.
    // If `depth` is non-null it receives the matched line's length in plies.
    const Opening* longest_match(const std::vector<std::string>& moves,
                                 std::size_t* depth = nullptr) const;

    std::size_t size() const { return openings_; }

    // A small built-in table of common openings, enough to classify games
    // that lack an ECO tag.
    static OpeningTrie with_builtin_openings();

private:
    struct Node {
        std::map<std::string, std::size_t> children;  // move -> index in nodes_
        std::optional<Opening> opening;
    };

    std::vector<Node> nodes_;  // nodes_[0] is the root
    std::size_t openings_ = 0;
};

}  // namespace pgnstats
