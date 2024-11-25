CC = gcc
LEX = flex
OUTPUT = z3
LEX_FILE = z3.l
LEX_C_FILE = lex.yy.c

all: build

build: $(LEX_C_FILE)
	$(CC) $(LEX_C_FILE) -o $(OUTPUT)

$(LEX_C_FILE): $(LEX_FILE)
	$(LEX) $(LEX_FILE)

clean:
	del $(LEX_C_FILE) $(OUTPUT)
