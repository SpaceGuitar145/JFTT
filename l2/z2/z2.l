%{
#include <stdio.h>

%}

%option noyywrap

%x STRING

%%

\"                  BEGIN(STRING); printf("%s", yytext);
\-\-.*$               ;
.|\n                printf("%s", yytext);
<STRING>{
    \"\"            printf("%s", yytext);
    \"              BEGIN(INITIAL); printf("%s", yytext);
    .               printf("%s", yytext);
    \n              printf("\n");
}


%%

int main(int argc, char **argv) {
    FILE *file = fopen(argv[1], "r");

    yyin = file;
    yylex();
    fclose(file);

    return 0;
}
