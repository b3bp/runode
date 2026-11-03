#define main lexer_demo_main
#include "../base/base_lexer.cpp"
#undef main
#include <iomanip>
#include <iterator>
int main() {
    std::string source{std::istreambuf_iterator<char>(std::cin), {}};
    for (const auto& token : Lexer(' ').get(source)) {
        if (token.type == TokenType::Empty || token.type == TokenType::Comment) continue;
        if (token.type == TokenType::Special && token.value.size() == 1 &&
            std::isspace(static_cast<unsigned char>(token.value[0]))) continue;
        for (unsigned char c : token.value)
            std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(c);
        std::cout << '\n';
    }
}
