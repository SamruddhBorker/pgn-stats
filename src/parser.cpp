#include "pgnstats/parser.hpp"

#include <algorithm>
#include <cctype>

namespace pgnstats {
namespace {

bool is_move_number(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return std::isdigit(c) != 0;
    });
}

bool is_result_symbol(const std::string& s) {
    return s == "1-0" || s == "0-1" || s == "1/2-1/2";
}

}  // namespace

Result parse_result(std::string_view text) {
    if (text == "1-0") return Result::WhiteWin;
    if (text == "0-1") return Result::BlackWin;
    if (text == "1/2-1/2") return Result::Draw;
    return Result::Unknown;
}

std::string_view to_string(Result result) {
    switch (result) {
        case Result::WhiteWin: return "1-0";
        case Result::BlackWin: return "0-1";
        case Result::Draw:     return "1/2-1/2";
        case Result::Unknown:  break;
    }
    return "*";
}

const std::string* Game::tag(std::string_view name) const {
    for (const auto& [key, value] : tags)
        if (key == name) return &value;
    return nullptr;
}

std::string Game::tag_or(std::string_view name, std::string fallback) const {
    const std::string* value = tag(name);
    return value && !value->empty() && *value != "?" ? *value : fallback;
}

std::string normalize_san(std::string_view san) {
    std::string out(san);
    while (!out.empty() && (out.back() == '+' || out.back() == '#' ||
                            out.back() == '!' || out.back() == '?'))
        out.pop_back();
    if (out == "0-0") return "O-O";
    if (out == "0-0-0") return "O-O-O";
    return out;
}

const Token& PgnParser::peek() {
    if (!lookahead_) lookahead_ = tokens_.next();
    return *lookahead_;
}

Token PgnParser::take() {
    Token tok = peek();
    lookahead_.reset();
    return tok;
}

void PgnParser::parse_tag(Game& game) {
    take();  // '['
    if (peek().type == TokenType::Symbol) {
        std::string name = take().text;
        if (peek().type == TokenType::String) {
            game.tags.emplace_back(std::move(name), take().text);
        }
    }
    // Recover from malformed tags by skipping to the closing bracket.
    while (peek().type != TokenType::RBracket && peek().type != TokenType::End) take();
    if (peek().type == TokenType::RBracket) take();
}

std::optional<Game> PgnParser::next() {
    Game game;
    bool started = false;     // saw anything belonging to this game
    bool in_movetext = false; // saw at least one movetext token
    int depth = 0;            // variation nesting level
    Result movetext_result = Result::Unknown;
    bool terminated = false;
    bool terminated_by_result = false;

    while (!terminated) {
        const Token& tok = peek();
        switch (tok.type) {
            case TokenType::End:
                terminated = true;
                break;

            case TokenType::LBracket:
                // A tag after movetext means the previous game lacked a
                // result token; end it here and leave the tag for next time.
                if (in_movetext) {
                    terminated = true;
                    break;
                }
                started = true;
                parse_tag(game);
                break;

            case TokenType::LParen:
                take();
                started = in_movetext = true;
                ++depth;
                ++game.variations;
                break;

            case TokenType::RParen:
                take();
                if (depth > 0) --depth;
                break;

            case TokenType::Comment:
                take();
                ++game.comments;
                break;

            case TokenType::Nag:
                take();
                ++game.nags;
                break;

            case TokenType::Asterisk:
                take();
                started = in_movetext = true;
                if (depth == 0) terminated = terminated_by_result = true;
                break;

            case TokenType::Symbol: {
                Token sym = take();
                started = in_movetext = true;
                if (is_result_symbol(sym.text)) {
                    if (depth == 0) {
                        movetext_result = parse_result(sym.text);
                        terminated = terminated_by_result = true;
                    }
                } else if (depth == 0 && !is_move_number(sym.text)) {
                    game.moves.push_back(normalize_san(sym.text));
                }
                break;
            }

            case TokenType::Period:
            case TokenType::RBracket:
            case TokenType::String:
                take();
                break;
        }
    }

    if (!started) return std::nullopt;

    // The movetext result terminates the game and wins over the tag; the
    // Result tag only fills in when the movetext was cut off.
    game.result = movetext_result;
    if (!terminated_by_result) {
        if (const std::string* tagged = game.tag("Result")) game.result = parse_result(*tagged);
    }
    return game;
}

}  // namespace pgnstats
