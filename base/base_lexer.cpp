#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

enum class TokenType { Keyword, Id, TextLiteral, NumberLiteral, Special, Comment, TemplateLiteral, Empty, UnclosedQuote, UnclosedComment, UnclosedTemplate, InvalidNumber };

enum class NumberForm { Integer, Fractional };
enum class NumberSign { Positive, Negative, Zero };

struct NumberCharacteristics {
    NumberForm form;
    NumberSign sign;
    bool operator==(const NumberCharacteristics&) const = default;
};

struct TokenPosition {
    std::size_t tokenNumber;
    std::size_t line;
    std::size_t column;
    std::size_t byteOffset;
    bool operator==(const TokenPosition&) const = default;
};

struct Token {
    std::string value;
    TokenType type;
    std::optional<TokenPosition> position = std::nullopt;

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
        const std::size_t digitStart = value.front() == '-' ? 1 : 0;
        if (digitStart < value.size() &&
            ((value[digitStart] >= '0' && value[digitStart] <= '9') || value[digitStart] == '.') &&
            std::all_of(value.begin() + digitStart, value.end(), [](char c) { return (c >= '0' && c <= '9') || c == '.'; }) &&
            std::any_of(value.begin() + digitStart, value.end(), [](char c) { return c >= '0' && c <= '9'; }) &&
            std::count(value.begin() + digitStart, value.end(), '.') > 1) return TokenType::InvalidNumber;
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
        std::vector<int> templateDepth;
        for (std::size_t i = 0; i < input.size(); ++i) {
            char character = input[i];
            if (quote != '\0') {
                current += character;
                if (escaped) escaped = false;
                else if (character == '\\') escaped = true;
                else if (character == quote) {
                    quote = '\0';
                    if (i + 1 < input.size() && !isDelimiter(input[i + 1])) {
                        result.push_back({current, typeOf_.get(current, false)});
                        current.clear();
                    }
                }
            } else if (character == '\"' || character == '\'') {
                if (!current.empty()) {
                    result.push_back({current, typeOf_.get(current, false)});
                    current.clear();
                }
                quote = character;
                current += character;
            } else if (character == '`' || (character == '}' && !templateDepth.empty() && templateDepth.back() == 0)) {
                if (!current.empty()) { result.push_back({current, typeOf_.get(current, false)}); current.clear(); }
                const bool isHead = character == '`';
                std::size_t end = i + 1;
                bool escape = false, closed = false, interpolation = false;
                while (end < input.size()) {
                    const char c = input[end];
                    if (escape) { escape = false; ++end; continue; }
                    if (c == '\\') { escape = true; ++end; continue; }
                    if (c == '`') { ++end; closed = true; break; }
                    if (c == '$' && end + 1 < input.size() && input[end + 1] == '{') {
                        end += 2; interpolation = true; break;
                    }
                    ++end;
                }
                result.push_back({input.substr(i, end - i),
                    (closed || interpolation) ? TokenType::TemplateLiteral : TokenType::UnclosedTemplate});
                if (isHead && interpolation) templateDepth.push_back(0);
                else if (!isHead && !interpolation) templateDepth.pop_back();
                i = end - 1;
            } else if (character == '/' && i + 1 < input.size() && (input[i + 1] == '/' || input[i + 1] == '*')) {
                if (!current.empty()) { result.push_back({current, typeOf_.get(current, false)}); current.clear(); }
                const bool lineComment = input[i + 1] == '/';
                std::size_t end = i + 2;
                if (lineComment) {
                    while (end < input.size() && input[end] != '\n' && input[end] != '\r') ++end;
                    result.push_back({input.substr(i, end - i), TokenType::Comment});
                } else {
                    while (end + 1 < input.size() && !(input[end] == '*' && input[end + 1] == '/')) ++end;
                    const bool closed = end + 1 < input.size();
                    end = closed ? end + 2 : input.size();
                    result.push_back({input.substr(i, end - i), closed ? TokenType::Comment : TokenType::UnclosedComment});
                }
                i = end - 1;
            } else if (character == '-' && delimiter_ != '-' && current.empty() &&
                       startsNumber(input, i + 1) && unaryPosition(input, i)) {
                current += character;
            } else if (character == '.' && (current.empty() ? (i + 1 < input.size() && std::isdigit(static_cast<unsigned char>(input[i + 1]))) :
                       (std::isdigit(static_cast<unsigned char>(current[0])) || current[0] == '-'))) {
                current += character;
            } else if (character == '.' && input.size() == 1) {
                current += character;
            } else if (isDelimiter(character)) {
                if (!templateDepth.empty() && character == '{') ++templateDepth.back();
                else if (!templateDepth.empty() && character == '}' && templateDepth.back() > 0) --templateDepth.back();
                result.push_back({current, typeOf_.get(current, false)});
                current.clear();
                static const std::vector<std::string> multiOperators = {
                    "===", "!==", "**=", "&&=", "||=", "?" "?=", "=>", "==", "!=", "++", "--", "**", ">=", "<=", "&&", "||", "??", "?.", "+=", "-=", "*=", "/="
                };
                std::string separator(1, character);
                for (const auto& op : multiOperators) {
                    if (input.compare(i, op.size(), op) == 0) { separator = op; i += op.size() - 1; break; }
                }
                result.push_back({separator, TokenType::Special});
            } else {
                current += character;
            }
        }
        result.push_back({current, quote != '\0' ? TokenType::UnclosedQuote : typeOf_.get(current, false)});
        if (!templateDepth.empty()) result.push_back({"", TokenType::UnclosedTemplate});
        std::size_t line = 1, column = 1, offset = 0, tokenNumber = 0;
        bool previousCR = false;
        for (auto& token : result) {
            if (!token.value.empty()) token.position = TokenPosition{++tokenNumber, line, column, offset};
            for (char ch : token.value) {
                ++offset;
                if (ch == '\r') { ++line; column = 1; previousCR = true; }
                else if (ch == '\n') { if (!previousCR) ++line; column = 1; previousCR = false; }
                else { ++column; previousCR = false; }
            }
        }
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
               std::string("+-*/=;:(){}[],\\?.<>!&|%^").find(character) != std::string::npos;
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
        case TokenType::Comment: return "comment";
        case TokenType::TemplateLiteral: return "template-literal";
        case TokenType::UnclosedTemplate: return "error: unclosed template";
        case TokenType::UnclosedComment: return "error: unclosed comment";
        case TokenType::Empty: return "empty";
        case TokenType::UnclosedQuote: return "error: unclosed quote";
        case TokenType::InvalidNumber: return "error: multiple decimal points";
    }
    return "unknown";
}

// Parser view retains line breaks and continuation markers; token numbers remain unchanged.
std::vector<Token> parserTokens(const std::vector<Token>& tokens) {
    std::vector<Token> result;
    for (const auto& token : tokens) {
        if (token.type == TokenType::Empty || token.type == TokenType::Comment) continue;
        if (token.type == TokenType::Special && token.value.size() == 1 &&
            token.value[0] != '\n' && token.value[0] != '\r' &&
            std::isspace(static_cast<unsigned char>(token.value[0]))) continue;
        result.push_back(token);
    }
    return result;
}

struct LexicalDiagnostic {
    TokenType error;
    std::string value;
    std::optional<TokenPosition> position;
};

std::vector<LexicalDiagnostic> lexicalDiagnostics(const std::vector<Token>& tokens) {
    std::vector<LexicalDiagnostic> result;
    for (const auto& token : tokens) {
        if (token.type == TokenType::UnclosedQuote || token.type == TokenType::UnclosedComment || token.type == TokenType::UnclosedTemplate || token.type == TokenType::InvalidNumber)
            result.push_back({token.type, token.value, token.position});
    }
    return result;
}

// A continuation marker must be immediately followed by a line break.
bool isContinuation(const std::vector<Token>& tokens, std::size_t index) {
    if (tokens[index].type != TokenType::Special || tokens[index].value != "\\") return false;
    ++index;
    while (index < tokens.size() && tokens[index].type == TokenType::Empty) ++index;
    return index < tokens.size() && tokens[index].type == TokenType::Special &&
           (tokens[index].value == "\n" || tokens[index].value == "\r");
}

// Preliminary sequential declaration check; scopes and forward declarations are not modeled.
std::vector<std::string> undeclaredIdentifiers(const std::vector<Token>& tokens) {
    std::unordered_set<std::string> declared;
    std::vector<std::string> errors;
    bool awaitingName = false;
    bool awaitingType = false;
    const std::unordered_set<std::string> builtInTypes{"number", "string", "boolean"};
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (isContinuation(tokens, i)) continue;
        if (token.type == TokenType::Empty || token.type == TokenType::Comment ||
            (token.type == TokenType::Special && token.value.size() == 1 &&
             std::isspace(static_cast<unsigned char>(token.value[0])))) continue;
        if (token.type == TokenType::Keyword && token.value == "let") {
            awaitingName = true;
            awaitingType = false;
        } else if (token.type == TokenType::Special && token.value == ":") {
            awaitingName = false;
            awaitingType = true;
        } else if (token.type == TokenType::Id) {
            if (awaitingName) declared.insert(token.value);
            else if (!(awaitingType && builtInTypes.contains(token.value)) &&
                     !declared.contains(token.value)) errors.push_back(token.value);
            awaitingName = false;
            awaitingType = false;
        } else {
            awaitingName = false;
            awaitingType = false;
        }
    }
    return errors;
}

// Preliminary instruction-ending check. Returns token indexes where the ending is missing.
std::vector<std::size_t> missingTerminators(const std::vector<Token>& tokens, char terminator = ';') {
    std::vector<std::size_t> errors;
    bool active = false;
    std::size_t last = 0;
    bool continuation = false;
    bool continuedCR = false;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (token.type == TokenType::Empty || token.type == TokenType::Comment) continue;
        if (isContinuation(tokens, i)) { continuation = true; continue; }
        if (token.type == TokenType::Special && token.value.size() == 1) {
            const char c = token.value[0];
            if (c == terminator) { active = false; continue; }
            if (c == '\n' || c == '\r') {
                if (continuation) {
                    continuation = false;
                    continuedCR = c == '\r';
                } else if (c == '\n' && continuedCR) {
                    continuedCR = false;
                } else if (active) {
                    errors.push_back(i);
                    active = false;
                }
                continue;
            }
            continuedCR = false;
            if (std::isspace(static_cast<unsigned char>(c))) continue;
        }
        if (token.type == TokenType::Keyword && token.value == "let" && active) {
            errors.push_back(i);
            active = false;
        }
        active = true;
        last = i;
    }
    if (active) errors.push_back(last + 1);
    return errors;
}

// Legacy test expectations compare token contents separately from source coordinates.
std::vector<Token> withoutPositions(std::vector<Token> tokens) {
    for (auto& token : tokens) token.position.reset();
    return tokens;
}

int main() {
    const Lexer lexer(' ');
    const auto positionedTokens = lexer.get("let a + b let d+b let c;");
    const auto tokens = withoutPositions(positionedTokens);
    const std::vector<Token> expected = {
        {"let", TokenType::Keyword}, {" ", TokenType::Special}, {"a", TokenType::Id},
        {" ", TokenType::Special}, {"", TokenType::Empty}, {"+", TokenType::Special},
        {"", TokenType::Empty}, {" ", TokenType::Special}, {"b", TokenType::Id},
        {" ", TokenType::Special}, {"let", TokenType::Keyword}, {" ", TokenType::Special},
        {"d", TokenType::Id}, {"+", TokenType::Special}, {"b", TokenType::Id},
        {" ", TokenType::Special}, {"let", TokenType::Keyword}, {" ", TokenType::Special},
        {"c", TokenType::Id}, {";", TokenType::Special}, {"", TokenType::Empty}
    };
    for (const auto& token : positionedTokens) {
        std::cout << '[' << token.value << "] " << typeName(token.type);
        if (token.position) {
            std::cout << " token=" << token.position->tokenNumber
                      << " line=" << token.position->line
                      << " column=" << token.position->column;
        } else {
            std::cout << " (empty, no token number)";
        }
        std::cout << '\n';
    }
    for (const auto& name : undeclaredIdentifiers(tokens)) {
        std::cout << "error: undeclared identifier [" << name << "]\n";
    }
    for (const auto index : missingTerminators(tokens)) {
        std::cout << "error: missing instruction terminator at element index " << index << '\n';
    }
    const std::string tsSource = "let count: number = 2;\nlet total: number = count + 3;\nlet label: string = \"total\";";
    const auto tsTokens = lexer.get(tsSource);
    const auto tsFiltered = parserTokens(tsTokens);
    const std::vector<Token> tsExpected = {
        {"let", TokenType::Keyword}, {"count", TokenType::Id}, {":", TokenType::Special},
        {"number", TokenType::Id}, {"=", TokenType::Special}, {"2", TokenType::NumberLiteral},
        {";", TokenType::Special}, {"\n", TokenType::Special},
        {"let", TokenType::Keyword}, {"total", TokenType::Id}, {":", TokenType::Special},
        {"number", TokenType::Id}, {"=", TokenType::Special}, {"count", TokenType::Id},
        {"+", TokenType::Special}, {"3", TokenType::NumberLiteral}, {";", TokenType::Special},
        {"\n", TokenType::Special}, {"let", TokenType::Keyword}, {"label", TokenType::Id},
        {":", TokenType::Special}, {"string", TokenType::Id}, {"=", TokenType::Special},
        {"\"total\"", TokenType::TextLiteral}, {";", TokenType::Special}
    };
    std::cout << "TypeScript example:\n" << tsSource << '\n';
    const auto templateTokens = parserTokens(lexer.get("`hello ${name}`"));
    const auto commentTokens = lexer.get("let x; /* missing");
    const auto adjacent = lexer.get("a\"hi\"b");
    const auto errors = lexicalDiagnostics(lexer.get("x;\n1.2.3"));
    const auto filtered = parserTokens(lexer.get("let a;\nb;"));
    const auto positions = lexer.get("a;\r\nb");
    const auto multiline = lexer.get("\"a\nb\";x");
    if (withoutPositions(templateTokens) != std::vector<Token>{{"`hello ${", TokenType::TemplateLiteral}, {"name", TokenType::Id}, {"}`", TokenType::TemplateLiteral}} ||
        lexicalDiagnostics(commentTokens).size() != 1 ||
        lexicalDiagnostics(commentTokens)[0].error != TokenType::UnclosedComment ||
        lexicalDiagnostics(lexer.get("`unfinished")).size() != 1 ||
        lexicalDiagnostics(lexer.get("`unfinished"))[0].error != TokenType::UnclosedTemplate ||
        withoutPositions(tsFiltered) != tsExpected ||
        !lexicalDiagnostics(tsTokens).empty() ||
        !undeclaredIdentifiers(tsTokens).empty() ||
        !missingTerminators(tsTokens).empty() ||
        tsFiltered[8].position->line != 2 || tsFiltered[18].position->line != 3 ||
        withoutPositions(adjacent) != std::vector<Token>{{"a", TokenType::Id}, {"\"hi\"", TokenType::TextLiteral}, {"b", TokenType::Id}} ||
        errors.size() != 1 || errors[0].error != TokenType::InvalidNumber ||
        errors[0].position != TokenPosition{4, 2, 1, 3} ||
        filtered.size() != 6 || filtered[0].value != "let" || filtered[1].value != "a" ||
        filtered[2].value != ";" || filtered[3].value != "\n" || filtered[4].value != "b" ||
        filtered[0].position->tokenNumber != 1 || filtered[1].position->tokenNumber != 3 ||
        multiline.size() != 3 ||
        multiline[0].position != TokenPosition{1, 1, 1, 0} ||
        multiline[1].position != TokenPosition{2, 2, 3, 5} ||
        multiline[2].position != TokenPosition{3, 2, 4, 6} ||
        positions.size() != 7 ||
        positions[0].position != TokenPosition{1, 1, 1, 0} ||
        positions[1].position != TokenPosition{2, 1, 2, 1} ||
        positions[2].position.has_value() ||
        positions[3].position != TokenPosition{3, 1, 3, 2} ||
        positions[5].position != TokenPosition{4, 2, 1, 3} ||
        positions[6].position != TokenPosition{5, 2, 1, 4} ||
        tokens != expected ||
        !missingTerminators(withoutPositions(lexer.get("let a;\nlet b;"))).empty() ||
        !missingTerminators(withoutPositions(lexer.get("let a = 1 + \\\n2;"))).empty() ||
        !missingTerminators(withoutPositions(lexer.get("let a = 1 + \\\r\n2;"))).empty() ||
        missingTerminators(withoutPositions(lexer.get("let a = 1 +\n2;"))).empty() ||
        missingTerminators(withoutPositions(lexer.get("let a = 1 + \\"))).empty() ||
        !undeclaredIdentifiers(withoutPositions(lexer.get("let \\\na;"))).empty() ||
        missingTerminators(withoutPositions(lexer.get("let a\nlet b;"))).size() != 1 ||
        missingTerminators(withoutPositions(lexer.get("let a let b;"))).size() != 1 ||
        missingTerminators(withoutPositions(lexer.get("let a"))).size() != 1 ||
        !missingTerminators(withoutPositions(Lexer('|').get("let a|")), '|').empty() ||
        !missingTerminators(withoutPositions(lexer.get("\";\";"))).empty() ||
        withoutPositions(lexer.get("")) != std::vector<Token>{{"", TokenType::Empty}} ||
        withoutPositions(lexer.get("123")) != std::vector<Token>{{"123", TokenType::NumberLiteral}} ||
        withoutPositions(lexer.get("12.5")) != std::vector<Token>{{"12.5", TokenType::NumberLiteral}} ||
        withoutPositions(lexer.get(".5")) != std::vector<Token>{{".5", TokenType::NumberLiteral}} ||
        withoutPositions(lexer.get("12.")) != std::vector<Token>{{"12.", TokenType::NumberLiteral}} ||
        withoutPositions(lexer.get("-12.5")) != std::vector<Token>{{"-12.5", TokenType::NumberLiteral}} ||
        withoutPositions(lexer.get("-.5")) != std::vector<Token>{{"-.5", TokenType::NumberLiteral}} ||
        withoutPositions(lexer.get(".")) != std::vector<Token>{{".", TokenType::Id}} ||
        withoutPositions(lexer.get("1.2.3")) != std::vector<Token>{{"1.2.3", TokenType::InvalidNumber}} ||
        withoutPositions(lexer.get("\"unterminated")) != std::vector<Token>{{"\"unterminated", TokenType::UnclosedQuote}} ||
        withoutPositions(lexer.get("'unterminated")) != std::vector<Token>{{"'unterminated", TokenType::UnclosedQuote}} ||
        !undeclaredIdentifiers(withoutPositions(lexer.get("let a; a"))).empty() ||
        undeclaredIdentifiers(withoutPositions(lexer.get("a; let b; b"))) != std::vector<std::string>{"a"} ||
        withoutPositions(lexer.get("a-12")) != std::vector<Token>{{"a", TokenType::Id}, {"-", TokenType::Special}, {"12", TokenType::NumberLiteral}} ||
        withoutPositions(lexer.get("42")).front().numberCharacteristics() != NumberCharacteristics{NumberForm::Integer, NumberSign::Positive} ||
        withoutPositions(lexer.get("-42")).front().numberCharacteristics() != NumberCharacteristics{NumberForm::Integer, NumberSign::Negative} ||
        withoutPositions(lexer.get(".5")).front().numberCharacteristics() != NumberCharacteristics{NumberForm::Fractional, NumberSign::Positive} ||
        withoutPositions(lexer.get("-12.")).front().numberCharacteristics() != NumberCharacteristics{NumberForm::Fractional, NumberSign::Negative} ||
        withoutPositions(lexer.get("-0.0")).front().numberCharacteristics() != NumberCharacteristics{NumberForm::Fractional, NumberSign::Zero} ||
        withoutPositions(lexer.get("word")).front().numberCharacteristics().has_value() ||
        withoutPositions(lexer.get("123abc")) != std::vector<Token>{{"123abc", TokenType::Id}} ||
        withoutPositions(lexer.get("\"123\"")) != std::vector<Token>{{"\"123\"", TokenType::TextLiteral}} ||
        withoutPositions(lexer.get("\"a + b\"")) != std::vector<Token>{{"\"a + b\"", TokenType::TextLiteral}} ||
        withoutPositions(lexer.get("'let'")) != std::vector<Token>{{"'let'", TokenType::TextLiteral}} ||
        withoutPositions(lexer.get("let")) != std::vector<Token>{{"let", TokenType::Keyword}} ||
        withoutPositions(lexer.get("name")) != std::vector<Token>{{"name", TokenType::Id}} ||
        withoutPositions(lexer.get("\"123\"")).front().numberCharacteristics().has_value() ||
        withoutPositions(lexer.get("letx")) != std::vector<Token>{{"letx", TokenType::Id}} ||
        withoutPositions(Lexer(':').get("a:b")) != std::vector<Token>{{"a", TokenType::Id}, {":", TokenType::Special}, {"b", TokenType::Id}}) {
        std::cerr << "Lexer test failed\n";
        return 1;
    }
    std::cout << "TypeScript example tokenization and checks passed\n";
    std::cout << "Lexer tests passed\n";
    return 0;
}
