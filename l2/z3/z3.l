%{
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

bool keep_docs = false;
%}

%option noyywrap

%x SINGLE_LINE_COMMENT MULTI_LINE_COMMENT STRING ARROW SINGLE_LINE_COMMENT_KEEP_DOCS MULTI_LINE_COMMENT_KEEP_DOCS

%%

\"                              BEGIN(STRING); printf("%s", yytext);
\<\<                            printf("%s", yytext);
\<                              BEGIN(ARROW); printf("%s", yytext);
\/\/                            BEGIN(SINGLE_LINE_COMMENT);
^[ \t]*\/\/                     BEGIN(SINGLE_LINE_COMMENT);
\/\*                            BEGIN(MULTI_LINE_COMMENT);
^[ \t]*\/\*                     BEGIN(MULTI_LINE_COMMENT);
\/\*\*\/                        printf(" ");
^[ \t]*\/\*\*\/                 printf(" "); 
\/\/\!|\/\/\/                   {
                                    if (keep_docs) {
                                        printf("%s", yytext); 
                                        BEGIN(SINGLE_LINE_COMMENT_KEEP_DOCS);
                                    }
                                    else {
                                        BEGIN(SINGLE_LINE_COMMENT);
                                    }
                                }
^[ \t]*\/\/\!|\/\/\/            {
                                    if (keep_docs) {
                                        printf("%s", yytext); 
                                        BEGIN(SINGLE_LINE_COMMENT_KEEP_DOCS);
                                    }
                                    else {
                                        BEGIN(SINGLE_LINE_COMMENT);
                                    }
                                }
\/\*\!|\/\*\*                   {
                                    if (keep_docs) {
                                        printf("%s", yytext); 
                                        BEGIN(MULTI_LINE_COMMENT_KEEP_DOCS);
                                    }
                                    else {
                                        BEGIN(MULTI_LINE_COMMENT);
                                    }
                                }
^[ \t]*\/\*\!|\/\*\*            {
                                    if (keep_docs) {
                                        printf("%s", yytext); 
                                        BEGIN(MULTI_LINE_COMMENT_KEEP_DOCS);
                                    }
                                    else {
                                        BEGIN(MULTI_LINE_COMMENT);
                                    }
                                }

<STRING>{
    \"                          BEGIN(INITIAL); printf("%s", yytext);
    .                           printf("%s", yytext);
}

<ARROW>{
    \>                          BEGIN(INITIAL); printf("%s", yytext);
    .                           printf("%s", yytext);
}

<SINGLE_LINE_COMMENT>{
    \\\n                  ;
    \n                          BEGIN(INITIAL);
    .                           ;
}

<MULTI_LINE_COMMENT>{
    \*\/                        BEGIN(INITIAL);
    .|\n                        ;
}

<SINGLE_LINE_COMMENT_KEEP_DOCS>{
    \\\n                  printf("%s", yytext);
    \n                          printf("\n"); BEGIN(INITIAL);
    .                           printf("%s", yytext); 
}

<MULTI_LINE_COMMENT_KEEP_DOCS>{
    \*\/                        BEGIN(INITIAL); printf("%s", yytext);
    \n                          printf("\n"); BEGIN(INITIAL);
    .                           printf("%s", yytext);
}

.|\n                            printf("%s", yytext);

%%

int main(int argc, char **argv) {
    FILE *file;

    if (argc >= 2 && strcmp(argv[1], "--keep-docs") == 0) {
        keep_docs = true; 
        file = fopen(argv[2], "r");
    } else if (argc >= 2) {
        file = fopen(argv[1], "r");
    }

    yyin = file;
    yylex();
    fclose(file);

    return 0;
}
