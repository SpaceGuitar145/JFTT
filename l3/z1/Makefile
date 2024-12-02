CC = gcc

LEX_FILE = scanner.l
BISON_FILE = parser.y
LEX_C = lex.yy.c
BISON_C = parser.tab.c
BISON_H = parser.tab.h
EXECUTABLE = translator

all: $(EXECUTABLE)

$(EXECUTABLE): $(BISON_C) $(LEX_C)
	$(CC) -o $(EXECUTABLE) $(BISON_C) $(LEX_C)

$(BISON_C) $(BISON_H): $(BISON_FILE)
	bison -d $(BISON_FILE)

$(LEX_C): $(LEX_FILE) $(BISON_H)
	flex $(LEX_FILE)

clean:
	rm -f $(BISON_C) $(BISON_H) $(LEX_C) $(EXECUTABLE)

