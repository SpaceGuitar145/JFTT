%{
#include "parser.tab.h"
#include <stdio.h>
%}

%option noyywrap noinput nounput
%x COMMENT

NUMBER      [0-9]+

%%
\#                  BEGIN(COMMENT);
{NUMBER}            yylval.num = atoi(yytext); return NUMBER;
"+"                 return '+';
"-"                 return '-';
"*"                 return '*';
"/"                 return '/';
"^"                 return '^';
"("                 return '(';
")"                 return ')';
\n                  return '\n';
[ \t\n]             ;
.                   fprintf(stderr, "Błąd: zły symbol %s\n", yytext);

<COMMENT>{
    \\\n            ;
    \n              BEGIN(INITIAL);
    .               ;
}

%%