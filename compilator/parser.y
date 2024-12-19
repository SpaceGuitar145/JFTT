%{
#include <stdio.h>
#include <stdlib.h>

extern int yylex();
extern int yyparse();
extern FILE *yyin;
void yyerror(const char *s);

%}

%union {
    char* sval;
    int ival;
}

%token PROGRAM PROCEDURE IS PROGRAM_BEGIN END ENDIF ELSE IF THEN WHILE DO ENDWHILE
%token REPEAT UNTIL FOR FROM TO DOWNTO ENDFOR READ WRITE
%token IDENTIFIER NUM T ASSIGN EQUAL NEQUAL GREATER LESS GEQUAL LEQUAL
%token ADD SUB MUL DIV MOD COMMA LPAREN RPAREN LBRACKET RBRACKET SEMICOLON COLON

%left ADD SUB
%left MUL DIV MOD
%left EQUAL NEQUAL GREATER LESS GEQUAL LEQUAL

%start program_all

%%

program_all:
    procedures main
;

procedures:
    procedures PROCEDURE proc_head IS declarations PROGRAM_BEGIN commands END
    | procedures PROCEDURE proc_head IS PROGRAM_BEGIN commands END
    |
;

main:
    PROGRAM IS declarations PROGRAM_BEGIN commands END
    | PROGRAM IS PROGRAM_BEGIN commands END
;

commands:
    commands command
    | command
;

command:
    identifier ASSIGN expression SEMICOLON
    | IF condition THEN commands ELSE commands ENDIF
    | IF condition THEN commands ENDIF
    | WHILE condition DO commands ENDWHILE
    | REPEAT commands UNTIL condition SEMICOLON
    | FOR IDENTIFIER FROM value TO value DO commands ENDFOR
    | FOR IDENTIFIER FROM value DOWNTO value DO commands ENDFOR
    | proc_call SEMICOLON
    | READ IDENTIFIER SEMICOLON
    | WRITE value SEMICOLON
;

proc_head:
    IDENTIFIER LPAREN args_decl RPAREN
;

proc_call:
    IDENTIFIER LPAREN args RPAREN
;

declarations:
    declarations COMMA IDENTIFIER
    | declarations COMMA IDENTIFIER LBRACKET NUM COLON NUM RBRACKET
    | IDENTIFIER
    | IDENTIFIER LBRACKET NUM COLON NUM RBRACKET
;

args_decl:
    args_decl COMMA IDENTIFIER
    | args_decl COMMA T IDENTIFIER
    | IDENTIFIER
    | T IDENTIFIER
;

args:
    args COMMA IDENTIFIER
    | IDENTIFIER
;

expression:
    value
    | value ADD value
    | value SUB value
    | value MUL value
    | value DIV value
    | value MOD value
;

condition:
    value EQUAL value
    | value NEQUAL value
    | value GREATER value
    | value LESS value
    | value GEQUAL value
    | value LEQUAL value
;

value:
    NUM
    | identifier
;

identifier:
    IDENTIFIER
    | IDENTIFIER LBRACKET IDENTIFIER RBRACKET
    | IDENTIFIER LBRACKET NUM RBRACKET
;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            perror("Unable to open file");
            return 1;
        }
        yyin = file;
    }
    yyparse();
    return 0;
}
