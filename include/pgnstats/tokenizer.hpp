#pragma once

#include <cstddef>
#include <istream>
#include <string>

namespace pgnstats {

enum class TokenType {
    Symbol,    // moves, move numbers, results ("1-0"), tag names
    String,    // "quoted" tag values, escapes already resolved
    Period,    // '.'
    Asterisk,  // '*' (unfinished game result)
    LBracket,  // '['
    RBracket,  // ']'
    LParen,    // '(' starts a recursive annotation variation
    RParen,    // ')'
    Nag,       // "$12" or a suffix annotation such as "!?"
    Comment,   // {brace} or ;rest-of-line comment, delimiters stripped
    End,
};

struct Token {
    TokenType type = TokenType::End;
    std::string text;
    std::size_t line = 0;
};

// Splits a PGN character stream into tokens one at a time, so arbitrarily
// large databases can be processed without loading them into memory.
class Tokenizer {
public:
    explicit Tokenizer(std::istream& in) : in_(in) {}

    Token next();
    std::size_t line() const { return line_; }

private:
    int get();
    int peek();

    std::istream& in_;
    std::size_t line_ = 1;
    int prev_ = '\n';  // last character read; '\n' means "at start of a line"
};

}  // namespace pgnstats
