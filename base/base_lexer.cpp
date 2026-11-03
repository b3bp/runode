#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

enum class TokenType { Keyword, TextId, NumberId, Special, Empty };

struct Token {
    std::string value;
    TokenType type;
    bool operator==(const Token&) const = default;
};

class TypeOf {
public:
    TokenType get(const std::string& value, bool isDelimiter) const {
        if (value.empty()) return TokenType::Empty;
        if (isDelimiter) return TokenType::Special;
        if (value.find_first_of("\"'") != std::string::npos) return TokenType::TextId;
        if (keywords_.contains(value)) return TokenType::Keyword;
        if (std::all_of(value.begin(), value.end(), [](char c) { return c >= '0' && c <= '9'; }))
            return TokenType::NumberId;
        return TokenType::TextId;
    }

private:
    const std::unordered_set<std::string> keywords_{"let"};
};

class Lexer {
public:
    explicit Lexer(char delimiter) : delimiter_(delimiter) {}

    std::vector<Token> get(const std::string& input) const {
        std::vector<Token> result;
        std::string current;
        char quote = '\0';
        bool escaped = false;
        for (char character : input) {
            if (quote != '\0') {
                current += character;
                if (escaped) escaped = false;
                else if (character == '\\') escaped = true;
                else if (character == quote) quote = '\0';
            } else if (character == '\"' || character == '\'') {
                quote = character;
                current += character;
            } else if (isDelimiter(character)) {
                result.push_back({current, typeOf_.get(current, false)});
                current.clear();
                std::string separator(1, character);
                result.push_back({separator, typeOf_.get(separator, true)});
            } else {
                current += character;
            }
        }
        result.push_back({current, typeOf_.get(current, false)});
        return result;
    }

private:
    bool isDelimiter(char character) const {
        return character == delimiter_ ||
               std::isspace(static_cast<unsigned char>(character)) ||
               std::string("+-*/=;(){}[],").find(character) != std::string::npos;
    }
    char delimiter_;
    TypeOf typeOf_;
};

const char* typeName(TokenType type) {
    switch (type) {
        case TokenType::Keyword: return "keyword";
        case TokenType::TextId: return "text-id";
        case TokenType::NumberId: return "number-id";
        case TokenType::Special: return "special";
        case TokenType::Empty: return "empty";
    }
    return "unknown";
}

int main() {
    const Lexer lexer(' ');
    const auto tokens = lexer.get("let a + b let d+b let c;");
    const std::vector<Token> expected = {
        {"let", TokenType::Keyword}, {" ", TokenType::Special}, {"a", TokenType::TextId},
        {" ", TokenType::Special}, {"", TokenType::Empty}, {"+", TokenType::Special},
        {"", TokenType::Empty}, {" ", TokenType::Special}, {"b", TokenType::TextId},
        {" ", TokenType::Special}, {"let", TokenType::Keyword}, {" ", TokenType::Special},
        {"d", TokenType::TextId}, {"+", TokenType::Special}, {"b", TokenType::TextId},
        {" ", TokenType::Special}, {"let", TokenType::Keyword}, {" ", TokenType::Special},
        {"c", TokenType::TextId}, {";", TokenType::Special}, {"", TokenType::Empty}
    };
    for (const auto& token : tokens) {
        std::cout << '[' << token.value << "] " << typeName(token.type) << '\n';
    }
    if (tokens != expected ||
        lexer.get("") != std::vector<Token>{{"", TokenType::Empty}} ||
        lexer.get("123") != std::vector<Token>{{"123", TokenType::NumberId}} ||
        lexer.get("123abc") != std::vector<Token>{{"123abc", TokenType::TextId}} ||
        lexer.get("\"123\"") != std::vector<Token>{{"\"123\"", TokenType::TextId}} ||
        lexer.get("\"a + b\"") != std::vector<Token>{{"\"a + b\"", TokenType::TextId}} ||
        lexer.get("'let'") != std::vector<Token>{{"'let'", TokenType::TextId}} ||
        lexer.get("letx") != std::vector<Token>{{"letx", TokenType::TextId}} ||
        Lexer(':').get("a:b") != std::vector<Token>{{"a", TokenType::TextId}, {":", TokenType::Special}, {"b", TokenType::TextId}}) {
        std::cerr << "Lexer test failed\n";
        return 1;
    }
    std::cout << "Lexer tests passed\n";
    return 0;
}
