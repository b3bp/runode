#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

enum class TokenType { Keyword, Id, TextLiteral, NumberLiteral, Special, Empty };

enum class NumberForm { Integer, Fractional };
enum class NumberSign { Positive, Negative, Zero };

struct NumberCharacteristics {
    NumberForm form;
    NumberSign sign;
    bool operator==(const NumberCharacteristics&) const = default;
};

struct Token {
    std::string value;
    TokenType type;

    std::optional<NumberCharacteristics> numberCharacteristics() const {
        if (type != TokenType::NumberLiteral) return std::nullopt;
        const NumberForm form = value.find('.') == std::string::npos
            ? NumberForm::Integer : NumberForm::Fractional;
        const bool zero = std::none_of(value.begin(), value.end(),
            [](char c) { return c >= '1' && c <= '9'; });
        const NumberSign sign = zero ? NumberSign::Zero
            : (value.front() == '-' ? NumberSign::Negative : NumberSign::Positive);
        return NumberCharacteristics{form, sign};
    }

    bool operator==(const Token&) const = default;
};

class TypeOf {
public:
    TokenType get(const std::string& value, bool isDelimiter) const {
        if (value.empty()) return TokenType::Empty;
        if (isDelimiter) return TokenType::Special;
        if (isQuoted(value)) return TokenType::TextLiteral;
        if (keywords_.contains(value)) return TokenType::Keyword;
        std::size_t start = value.front() == '-' ? 1 : 0;
        bool hasDigit = false;
        bool hasDot = false;
        bool number = start < value.size();
        for (std::size_t i = start; i < value.size(); ++i) {
            const char c = value[i];
            if (c >= '0' && c <= '9') hasDigit = true;
            else if (c == '.' && !hasDot) hasDot = true;
            else { number = false; break; }
        }
        if (number && hasDigit) return TokenType::NumberLiteral;
        return TokenType::Id;
    }

private:
    static bool isQuoted(const std::string& value) {
        if (value.size() < 2 || (value.front() != '\"' && value.front() != '\'')) return false;
        bool escaped = false;
        for (std::size_t i = 1; i < value.size(); ++i) {
            if (escaped) escaped = false;
            else if (value[i] == '\\') escaped = true;
            else if (value[i] == value.front()) return i == value.size() - 1;
        }
        return false;
    }
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
        for (std::size_t i = 0; i < input.size(); ++i) {
            char character = input[i];
            if (quote != '\0') {
                current += character;
                if (escaped) escaped = false;
                else if (character == '\\') escaped = true;
                else if (character == quote) quote = '\0';
            } else if (character == '\"' || character == '\'') {
                quote = character;
                current += character;
            } else if (character == '-' && delimiter_ != '-' && current.empty() &&
                       startsNumber(input, i + 1) && unaryPosition(input, i)) {
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
    static bool startsNumber(const std::string& input, std::size_t index) {
        if (index >= input.size()) return false;
        if (input[index] >= '0' && input[index] <= '9') return true;
        return input[index] == '.' && index + 1 < input.size() &&
               input[index + 1] >= '0' && input[index + 1] <= '9';
    }
    static bool unaryPosition(const std::string& input, std::size_t index) {
        while (index > 0 && std::isspace(static_cast<unsigned char>(input[index - 1]))) --index;
        return index == 0 || std::string("=([{,+-*/;").find(input[index - 1]) != std::string::npos;
    }
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
        case TokenType::TextLiteral: return "text-literal";
        case TokenType::NumberLiteral: return "number-literal";
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
        lexer.get("123") != std::vector<Token>{{"123", TokenType::NumberLiteral}} ||
        lexer.get("12.5") != std::vector<Token>{{"12.5", TokenType::NumberLiteral}} ||
        lexer.get(".5") != std::vector<Token>{{".5", TokenType::NumberLiteral}} ||
        lexer.get("12.") != std::vector<Token>{{"12.", TokenType::NumberLiteral}} ||
        lexer.get("-12.5") != std::vector<Token>{{"-12.5", TokenType::NumberLiteral}} ||
        lexer.get("-.5") != std::vector<Token>{{"-.5", TokenType::NumberLiteral}} ||
        lexer.get(".") != std::vector<Token>{{".", TokenType::Id}} ||
        lexer.get("1.2.3") != std::vector<Token>{{"1.2.3", TokenType::Id}} ||
        lexer.get("a-12") != std::vector<Token>{{"a", TokenType::Id}, {"-", TokenType::Special}, {"12", TokenType::NumberLiteral}} ||
        lexer.get("42").front().numberCharacteristics() != NumberCharacteristics{NumberForm::Integer, NumberSign::Positive} ||
        lexer.get("-42").front().numberCharacteristics() != NumberCharacteristics{NumberForm::Integer, NumberSign::Negative} ||
        lexer.get(".5").front().numberCharacteristics() != NumberCharacteristics{NumberForm::Fractional, NumberSign::Positive} ||
        lexer.get("-12.").front().numberCharacteristics() != NumberCharacteristics{NumberForm::Fractional, NumberSign::Negative} ||
        lexer.get("-0.0").front().numberCharacteristics() != NumberCharacteristics{NumberForm::Fractional, NumberSign::Zero} ||
        lexer.get("word").front().numberCharacteristics().has_value() ||
        lexer.get("123abc") != std::vector<Token>{{"123abc", TokenType::Id}} ||
        lexer.get("\"123\"") != std::vector<Token>{{"\"123\"", TokenType::TextLiteral}} ||
        lexer.get("\"a + b\"") != std::vector<Token>{{"\"a + b\"", TokenType::TextLiteral}} ||
        lexer.get("'let'") != std::vector<Token>{{"'let'", TokenType::TextLiteral}} ||
        lexer.get("let") != std::vector<Token>{{"let", TokenType::Keyword}} ||
        lexer.get("name") != std::vector<Token>{{"name", TokenType::Id}} ||
        lexer.get("\"123\"").front().numberCharacteristics().has_value() ||
        lexer.get("letx") != std::vector<Token>{{"letx", TokenType::Id}} ||
        Lexer(':').get("a:b") != std::vector<Token>{{"a", TokenType::Id}, {":", TokenType::Special}, {"b", TokenType::Id}}) {
        std::cerr << "Lexer test failed\n";
        return 1;
    }
    std::cout << "Lexer tests passed\n";
    return 0;
}
