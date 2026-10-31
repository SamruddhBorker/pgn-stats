#include <string>
#include <vector>

#include "doctest/doctest.h"
#include "pgnstats/opening_trie.hpp"

using pgnstats::OpeningTrie;

TEST_CASE("longest match picks the most specific opening") {
    OpeningTrie trie;
    trie.insert("e4", {"B00", "King's Pawn"});
    trie.insert("e4 e5 Nf3 Nc6 Bb5", {"C60", "Ruy Lopez"});
    CHECK(trie.size() == 2);

    std::size_t depth = 0;
    const auto* o = trie.longest_match({"e4", "e5", "Nf3", "Nc6", "Bb5", "a6"}, &depth);
    REQUIRE(o != nullptr);
    CHECK(o->eco == "C60");
    CHECK(depth == 5);

    // Diverging before the deeper entry falls back to the shallower one.
    o = trie.longest_match({"e4", "e5", "Nf3", "Nc6", "Bc4"}, &depth);
    REQUIRE(o != nullptr);
    CHECK(o->eco == "B00");
    CHECK(depth == 1);
}

TEST_CASE("no match returns nullptr") {
    OpeningTrie trie;
    trie.insert("e4", {"B00", "King's Pawn"});
    std::size_t depth = 99;
    CHECK(trie.longest_match({"d4"}, &depth) == nullptr);
    CHECK(depth == 0);
    CHECK(trie.longest_match({}) == nullptr);
}

TEST_CASE("re-inserting a line replaces it without growing the count") {
    OpeningTrie trie;
    trie.insert("d4 d5", {"D00", "old"});
    trie.insert("d4 d5", {"D00", "new"});
    CHECK(trie.size() == 1);
    CHECK(trie.longest_match({"d4", "d5"})->name == "new");
}

TEST_CASE("built-in table classifies well-known lines") {
    const OpeningTrie trie = OpeningTrie::with_builtin_openings();
    CHECK(trie.size() > 30);
    CHECK(trie.longest_match({"e4", "e5", "Nf3", "Nc6", "Bc4", "Bc5", "b4", "Bxb4"})->eco ==
          "C51");
    CHECK(trie.longest_match({"e4", "c5", "Nf3"})->eco == "B20");
    CHECK(trie.longest_match({"d4", "Nf6", "c4", "g6", "Nc3"})->eco == "E60");
    CHECK(trie.longest_match({"e4", "e5", "Bc4", "d6"})->name == "Bishop's Opening");
}
