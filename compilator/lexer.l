%{
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

%option noyywrap noinput nounput

%x COMMENT

%%

\#                          BEGIN(COMMENT);
PROCEDURE                   return PROCEDURE;
IF                          return IF;
THEN                        return THEN;
ELSE                        return ELSE;
ENDIF                       return ENDIF;
WHILE                       return WHILE;
DO                          return DO;
ENDWHILE                    return ENDWHILE;
REPEAT                      return REPEAT;
UNTIL                       return UNTIL;
FOR                         return FOR;
FROM                        return FROM;
TO                          return TO;
DOWNTO                      return DOWNTO;
ENDFOR                      return ENDFOR;
IS                          return IS;
BEGIN                       return PROGRAM_BEGIN;
END                         return END;
PROGRAM                     return PROGRAM;
READ                        return READ;
WRITE                       return WRITE;
T                           return T;
\;                          return SEMICOLON;
\,                          return COMMA;
":="                        return ASSIGN;
\=                          return EQUAL;
"!="                        return NEQUAL;
\>                          return GREATER;
\<                          return LESS;
">="                        return GEQUAL;
"<="                        return LEQUAL;
\+                          return ADD;
\-                          return SUB;
\*                          return MUL;
\/                          return DIV;
\%                          return MOD;
\(                          return LPAREN;
\)                          return RPAREN;
\[                          return LBRACKET;
\]                          return RBRACKET;
\:                          return COLON;
[_a-z]+                     yylval.sval = strdup(yytext); return IDENTIFIER;
[0-9]+                      yylval.ival = atoi(yytext); return NUM;
[ \t\r\n]+                  ;

<COMMENT>{
    \n                      BEGIN(INITIAL);
    .                       ;
}
%%
