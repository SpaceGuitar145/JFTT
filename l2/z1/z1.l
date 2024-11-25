%{
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int line_count = 0;
int word_count = 0;
bool is_blank = false;

%}

%option noyywrap

%%
^[ \t]*\n           is_blank = true;
\n                  line_count++; printf("\n");
^[ \t]+             ;
[ \t]+$             ;
[ \t]+              printf(" ");
[^ \t\n]*           word_count++; printf("%s", yytext); is_blank = false;
<<EOF>>             if (!is_blank) line_count++; return 0;

%%

int main(int argc, char **argv) {
    FILE *file = fopen(argv[1], "r");

    yyin = file;
    yylex();
    fclose(file);

    printf("\nLiczba linii: %d\n", line_count);
    printf("Liczba słów: %d\n", word_count);
    return 0;
}
