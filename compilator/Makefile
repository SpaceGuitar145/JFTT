CXX = g++
FLEX = flex
BISON = bison
CXXFLAGS = -std=c++17 -Wall -Wextra

LEXER = lexer.l
PARSER = parser.y
EXECUTABLE = kompilator

all: $(EXECUTABLE)

$(EXECUTABLE): parser.tab.o lex.yy.o main.o
	$(CXX) $(CXXFLAGS) -o $@ $^

parser.tab.o: parser.tab.c
	$(CXX) $(CXXFLAGS) -c $<

lex.yy.o: lex.yy.c
	$(CXX) $(CXXFLAGS) -c $<

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c $<

parser.tab.c parser.tab.h: $(PARSER)
	$(BISON) -d $<

lex.yy.c: $(LEXER) parser.tab.h
	$(FLEX) $<

clean:
	rm -f kompilator parser.tab.* lex.yy.* *.o
