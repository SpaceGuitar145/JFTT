CC = gcc
FLEX = flex
BISON = bison

LEXER_SRC = lexer.l
PARSER_SRC = parser.y

LEXER_OBJ = lex.yy.c
PARSER_OBJ = parser.tab.c
PARSER_HEADER = parser.tab.h

OUTPUT_EXEC = kompilator

all: $(OUTPUT_EXEC)

$(PARSER_OBJ) $(PARSER_HEADER): $(PARSER_SRC)
	$(BISON) -d -o $(PARSER_OBJ) $(PARSER_SRC)

$(LEXER_OBJ): $(LEXER_SRC) $(PARSER_HEADER)
	$(FLEX) -o $(LEXER_OBJ) $(LEXER_SRC)

$(OUTPUT_EXEC): $(PARSER_OBJ) $(LEXER_OBJ)
	$(CC) -o $(OUTPUT_EXEC) $(PARSER_OBJ) $(LEXER_OBJ)

clean:
	rm -f $(PARSER_OBJ) $(PARSER_HEADER) $(LEXER_OBJ) $(OUTPUT_EXEC)

.PHONY: all clean
