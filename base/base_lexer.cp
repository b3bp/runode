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
