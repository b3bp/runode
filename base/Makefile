CXX := clang++
CXXFLAGS := -std=c++23 -Wall -Wextra -Werror -pedantic

.PHONY: all test clean

all: base_lexer

base_lexer: base_lexer.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

test: base_lexer
	./base_lexer

clean:
	rm -f base_lexer
