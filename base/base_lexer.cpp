#include <cctype>
#include <iostream>
#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(char delimiter) : delimiter_(delimiter) {}

    std::vector<std::string> get(const std::string& input) const {
        std::vector<std::string> result;
        std::string current;
        for (char character : input) {
            if (isDelimiter(character)) {
                result.push_back(current);
                current.clear();
                result.emplace_back(1, character);
            } else {
                current += character;
            }
        }
        result.push_back(current);
        return result;
    }

private:
    bool isDelimiter(char character) const {
        return character == delimiter_ ||
               std::isspace(static_cast<unsigned char>(character)) ||
               std::string("+-*/=;(){}[],").find(character) != std::string::npos;
    }

    char delimiter_;
};

int main() {
    const Lexer lexer(' ');
    const std::string input = "let a + b let d+b let c;";
    const std::vector<std::string> expected = {
        "let", " ", "a", " ", "", "+", "", " ", "b", " ",
        "let", " ", "d", "+", "b", " ", "let", " ", "c", ";", ""
    };
    const auto tokens = lexer.get(input);
    for (const auto& token : tokens) {
        std::cout << '[' << token << "]\n";
    }
    if (tokens != expected ||
        lexer.get(" a  b ") != std::vector<std::string>{"", " ", "a", " ", "", " ", "b", " ", ""} ||
        lexer.get("") != std::vector<std::string>{""} ||
        lexer.get("a\tb\nc") != std::vector<std::string>{"a", "\t", "b", "\n", "c"} ||
        Lexer(':').get("a:b") != std::vector<std::string>{"a", ":", "b"}) {
        std::cerr << "Lexer test failed\n";
        return 1;
    }
    std::cout << "Lexer tests passed\n";
    return 0;
}
