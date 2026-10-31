#include "pgnstats/tokenizer.hpp"

#include <cctype>

namespace pgnstats {
namespace {

bool is_symbol_start(int c) {
    return c != EOF && std::isalnum(static_cast<unsigned char>(c));
}

bool is_symbol_continuation(int c) {
    if (c == EOF) return false;
    if (std::isalnum(static_cast<unsigned char>(c))) return true;
    switch (c) {
        case '_': case '+': case '#': case '=': case ':': case '-': case '/':
            return true;
        default:
            return false;
    }
}

}  // namespace

int Tokenizer::get() {
    const int c = in_.get();
    if (c == '\n') ++line_;
    if (c != EOF) prev_ = c;
    return c;
}

int Tokenizer::peek() { return in_.peek(); }

Token Tokenizer::next() {
    for (;;) {
        const bool line_start = prev_ == '\n';
        const int c = get();
        if (c == EOF) return {TokenType::End, {}, line_};
        if (std::isspace(static_cast<unsigned char>(c))) continue;

        Token tok{TokenType::End, {}, line_};

        // '%' in the first column escapes the whole line (PGN spec 6).
        if (c == '%' && line_start) {
            while (peek() != EOF && peek() != '\n') get();
            continue;
        }

        switch (c) {
            case '.': tok.type = TokenType::Period;   tok.text = "."; return tok;
            case '*': tok.type = TokenType::Asterisk; tok.text = "*"; return tok;
            case '[': tok.type = TokenType::LBracket; tok.text = "["; return tok;
            case ']': tok.type = TokenType::RBracket; tok.text = "]"; return tok;
            case '(': tok.type = TokenType::LParen;   tok.text = "("; return tok;
            case ')': tok.type = TokenType::RParen;   tok.text = ")"; return tok;

            case '"': {
                tok.type = TokenType::String;
                for (int d = get(); d != EOF && d != '"'; d = get()) {
                    if (d == '\\' && (peek() == '"' || peek() == '\\')) d = get();
                    tok.text.push_back(static_cast<char>(d));
                }
                return tok;
            }

            case '{': {
                tok.type = TokenType::Comment;
                for (int d = get(); d != EOF && d != '}'; d = get())
                    tok.text.push_back(static_cast<char>(d));
                return tok;
            }

            case ';': {
                tok.type = TokenType::Comment;
                while (peek() != EOF && peek() != '\n')
                    tok.text.push_back(static_cast<char>(get()));
                return tok;
            }

            case '$': {
                tok.type = TokenType::Nag;
                tok.text = "$";
                while (peek() != EOF && std::isdigit(static_cast<unsigned char>(peek())))
                    tok.text.push_back(static_cast<char>(get()));
                return tok;
            }

            case '!': case '?': {
                tok.type = TokenType::Nag;
                tok.text.push_back(static_cast<char>(c));
                while (peek() == '!' || peek() == '?')
                    tok.text.push_back(static_cast<char>(get()));
                return tok;
            }

            default:
                break;
        }

        if (is_symbol_start(c)) {
            tok.type = TokenType::Symbol;
            tok.text.push_back(static_cast<char>(c));
            while (is_symbol_continuation(peek()))
                tok.text.push_back(static_cast<char>(get()));
            return tok;
        }

        // Anything else ('<', '>', stray punctuation) is reserved or noise:
        // skip it rather than failing the whole file.
    }
}

}  // namespace pgnstats
