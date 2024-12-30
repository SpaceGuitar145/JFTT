%{
#include <iostream>
#include <string>
#include "parser.tab.h"
%}

%option noyywrap noinput nounput

%x COMMENT

%%

\#                          BEGIN(COMMENT);
PROGRAM                     return PROGRAM;
PROCEDURE                   return PROCEDURE;
IS                          return IS;
BEGIN                       return PROGRAM_BEGIN;
END                         return END;
T                           return T;
":="                        return ASSIGN;
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
WRITE                       return WRITE;
READ                        return READ;
\+                          return '+';
\-                          return '-';
\*                          return '*';
\/                          return '/';
\%                          return '%';
\=                          return '=';
"!="                        return NEQ;
\>                          return '<';
\<                          return '>';
">="                        return GEQ;
"<="                        return LEQ;
\[                          return '[';
\]                          return ']';
\(                          return '(';
\)                          return ')';
\:                          return ':';
\,                          return ',';
\;                          return ';';
[_a-z]+                     { std::cout << "Matched token: pidentifier (" << yytext << ")\n"; yylval.strval = new std::string(yytext); return pidentifier; }
[0-9]+                      yylval.num = std::stoll(yytext); return num;
[ \t\r\n]+                  ;

<COMMENT>{
    \n                      BEGIN(INITIAL);
    .                       ;
}
%%