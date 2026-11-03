#include <cctype>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

enum class TokenType { Keyword, Id, Special, Empty };

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
        if (keywords_.contains(value)) return TokenType::Keyword;
        return TokenType::Id;
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
        for (char character : input) {
            if (isDelimiter(character)) {
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
        case TokenType::Id: return "id";
        case TokenType::Special: return "special";
        case TokenType::Empty: return "empty";
    }
    return "unknown";
}

int main() {
    const Lexer lexer(' ');
    const auto tokens = lexer.get("let a + b let d+b let c;");
    const std::vector<Token> expected = {
        {"let", TokenType::Keyword}, {" ", TokenType::Special}, {"a", TokenType::Id},
        {" ", TokenType::Special}, {"", TokenType::Empty}, {"+", TokenType::Special},
        {"", TokenType::Empty}, {" ", TokenType::Special}, {"b", TokenType::Id},
        {" ", TokenType::Special}, {"let", TokenType::Keyword}, {" ", TokenType::Special},
        {"d", TokenType::Id}, {"+", TokenType::Special}, {"b", TokenType::Id},
        {" ", TokenType::Special}, {"let", TokenType::Keyword}, {" ", TokenType::Special},
        {"c", TokenType::Id}, {";", TokenType::Special}, {"", TokenType::Empty}
    };
    for (const auto& token : tokens) {
        std::cout << '[' << token.value << "] " << typeName(token.type) << '\n';
    }
    if (tokens != expected ||
        lexer.get("") != std::vector<Token>{{"", TokenType::Empty}} ||
        lexer.get("letx") != std::vector<Token>{{"letx", TokenType::Id}} ||
        Lexer(':').get("a:b") != std::vector<Token>{{"a", TokenType::Id}, {":", TokenType::Special}, {"b", TokenType::Id}}) {
        std::cerr << "Lexer test failed\n";
        return 1;
    }
    std::cout << "Lexer tests passed\n";
    return 0;
}
