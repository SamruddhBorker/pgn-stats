#include <sstream>
#include <vector>

#include "doctest/doctest.h"
#include "pgnstats/tokenizer.hpp"

using pgnstats::Token;
using pgnstats::Tokenizer;
using pgnstats::TokenType;

namespace {

std::vector<Token> tokenize(const std::string& text) {
    std::istringstream in(text);
    Tokenizer tokenizer(in);
    std::vector<Token> out;
    for (Token t = tokenizer.next(); t.type != TokenType::End; t = tokenizer.next())
        out.push_back(t);
    return out;
}

}  // namespace

TEST_CASE("tag pair tokens") {
    const auto t = tokenize(R"([White "Morphy, Paul"])");
    REQUIRE(t.size() == 4);
    CHECK(t[0].type == TokenType::LBracket);
    CHECK(t[1].type == TokenType::Symbol);
    CHECK(t[1].text == "White");
    CHECK(t[2].type == TokenType::String);
    CHECK(t[2].text == "Morphy, Paul");
    CHECK(t[3].type == TokenType::RBracket);
}

TEST_CASE("string escapes are resolved") {
    const auto t = tokenize(R"("say \"hi\" \\ bye")");
    REQUIRE(t.size() == 1);
    CHECK(t[0].text == R"(say "hi" \ bye)");
}

TEST_CASE("move numbers, periods and moves") {
    const auto t = tokenize("12. Nxe5+ 12... O-O-O");
    REQUIRE(t.size() == 8);
    CHECK(t[0].text == "12");
    CHECK(t[1].type == TokenType::Period);
    CHECK(t[2].text == "Nxe5+");
    CHECK(t[3].text == "12");
    CHECK(t[4].type == TokenType::Period);
    CHECK(t[5].type == TokenType::Period);
    CHECK(t[6].type == TokenType::Period);
    CHECK(t[7].text == "O-O-O");
}

TEST_CASE("promotion and results are single symbols") {
    const auto t = tokenize("e8=Q# 1/2-1/2 *");
    REQUIRE(t.size() == 3);
    CHECK(t[0].text == "e8=Q#");
    CHECK(t[1].text == "1/2-1/2");
    CHECK(t[2].type == TokenType::Asterisk);
}

TEST_CASE("comments, NAGs and suffix annotations") {
    const auto t = tokenize("e4 {best by test} $1 e5!? ; rest of line\nNf3");
    REQUIRE(t.size() == 7);
    CHECK(t[1].type == TokenType::Comment);
    CHECK(t[1].text == "best by test");
    CHECK(t[2].type == TokenType::Nag);
    CHECK(t[2].text == "$1");
    CHECK(t[3].text == "e5");
    CHECK(t[4].type == TokenType::Nag);
    CHECK(t[4].text == "!?");
    CHECK(t[5].type == TokenType::Comment);
    CHECK(t[5].text == " rest of line");
    CHECK(t[6].text == "Nf3");
}

TEST_CASE("variations produce paren tokens") {
    const auto t = tokenize("e4 (d4 (c4)) e5");
    REQUIRE(t.size() == 8);
    CHECK(t[1].type == TokenType::LParen);
    CHECK(t[3].type == TokenType::LParen);
    CHECK(t[5].type == TokenType::RParen);
    CHECK(t[6].type == TokenType::RParen);
}

TEST_CASE("percent escape only applies in column one") {
    const auto t = tokenize("% ignored line\ne4 % not-an-escape\n%also ignored\ne5");
    REQUIRE(t.size() == 3);
    CHECK(t[0].text == "e4");
    CHECK(t[1].text == "not-an-escape");
    CHECK(t[2].text == "e5");
}

TEST_CASE("line numbers are tracked") {
    std::istringstream in("e4\n\n  e5");
    Tokenizer tokenizer(in);
    CHECK(tokenizer.next().line == 1);
    CHECK(tokenizer.next().line == 3);
}

TEST_CASE("unterminated comment ends at EOF without hanging") {
    const auto t = tokenize("e4 { never closed");
    REQUIRE(t.size() == 2);
    CHECK(t[1].type == TokenType::Comment);
}
