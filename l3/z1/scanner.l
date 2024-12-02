%{
#include "parser.tab.h"
#include <stdio.h>
%}

%option noyywrap noinput nounput
%x COMMENT

DIGIT      [0-9]+

%%
\#                  BEGIN(COMMENT);
{DIGIT}             yylval.num = atoi(yytext); return NUMBER;
"+"                 return '+';
"-"                 return '-';
"*"                 return '*';
"/"                 return '/';
"^"                 return '^';
"("                 return '(';
")"                 return ')';
\n                  return '\n';
[ \t\n]             ;
.                   fprintf(stderr, "Unknown character: %s\n", yytext);

<COMMENT>{
    \\\n            ;
    \n              BEGIN(INITIAL);
    .               ;
}

%%