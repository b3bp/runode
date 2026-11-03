#include <string>
#include <vector>

class Lexer {
public:
    explicit Lexer(char delimiter) : delimiter_(delimiter) {}

    std::vector<std::string> get(const std::string& input) const {
        std::vector<std::string> result;
        std::string current;
        for (char character : input) {
            if (character == delimiter_) {
                result.push_back(current);
                current.clear();
            } else {
                current += character;
            }
        }
        result.push_back(current);
        return result;
    }

private:
    char delimiter_;
};

#include <iostream>

int main() {
    const Lexer lexer(' ');
    const std::string input = "let a + b let d+b let c;";
    const std::vector<std::string> expected = {
        "let", "a", "+", "b", "let", "d+b", "let", "c;"
    };
    const auto tokens = lexer.get(input);
    for (const auto& token : tokens) {
        std::cout << '[' << token << "]\n";
    }
    if (tokens != expected ||
        lexer.get(" a  b ") != std::vector<std::string>{"", "a", "", "b", ""} ||
        lexer.get("") != std::vector<std::string>{""}) {
        std::cerr << "Lexer test failed\n";
        return 1;
    }
    std::cout << "Lexer tests passed\n";
    return 0;
}
