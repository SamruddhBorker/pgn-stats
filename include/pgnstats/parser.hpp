#pragma once

#include <cstddef>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pgnstats/tokenizer.hpp"

namespace pgnstats {

enum class Result { WhiteWin, BlackWin, Draw, Unknown };

// "1-0", "0-1", "1/2-1/2"; anything else (including "*") is Unknown.
Result parse_result(std::string_view text);
std::string_view to_string(Result result);

struct Game {
    std::vector<std::pair<std::string, std::string>> tags;  // in file order
    std::vector<std::string> moves;  // main line only, normalized SAN
    Result result = Result::Unknown;
    std::size_t comments = 0;
    std::size_t nags = 0;
    std::size_t variations = 0;

    // Value of the first tag called `name`, or nullptr.
    const std::string* tag(std::string_view name) const;
    std::string tag_or(std::string_view name, std::string fallback) const;
};

// Strips check/mate marks and annotation glyphs and maps "0-0" to "O-O",
// so the same move always compares equal.
std::string normalize_san(std::string_view san);

// Reads games one at a time from a PGN stream. Comments, NAGs and
// variations are counted but kept out of the main line.
class PgnParser {
public:
    explicit PgnParser(std::istream& in) : tokens_(in) {}

    std::optional<Game> next();

private:
    const Token& peek();
    Token take();
    void parse_tag(Game& game);

    Tokenizer tokens_;
    std::optional<Token> lookahead_;
};

}  // namespace pgnstats
