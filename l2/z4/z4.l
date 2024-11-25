%{
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define STACK_SIZE 20

int pointer = 0;
int stack[STACK_SIZE];
int a, b;

void push(int value);
int pop();

%}

%option noyywrap

%x RESET

%%

\-?[0-9]+       push(atoi(yytext));

\+              b = pop(); a = pop(); push(a + b);
\-              b = pop(); a = pop(); push(a - b);
\*              b = pop(); a = pop(); push(a * b);
\^              b = pop(); a = pop(); push((int)pow(a, b));

\/              {
                    b = pop();
                    a = pop();
                    if (b != 0) {
                        push(a / b);
                    } else {
                        printf("Błąd: dzielenie przez 0\n");
                        BEGIN(RESET);
                    }
                }

\%              {
                    b = pop();
                    a = pop();
                    if (b != 0) {
                        push(a % b);
                    } else {
                        printf("Błąd: dzielenie przez 0\n");
                        BEGIN(RESET);
                    }
                }

\n              {
                    if (pointer != 1) {
                        printf("Błąd: za mała liczba operatorów\n");
                    } else {
                        printf("= %d\n", pop());
                    }
                    pointer = 0;
                }

<RESET>{
    .           ;
    \n          pointer = 0; BEGIN(INITIAL);
}

[ \t]+          ;
.               printf("Błąd: zły symbol \"%s\" \n", yytext); BEGIN(RESET);

%%

void push(int value) {
    if (pointer >= STACK_SIZE) {
        printf("Błąd: przepełnienie stosu\n");
        BEGIN(RESET);
    } else {
        stack[pointer++] = value;
    }
}

int pop() {
    if (pointer > 0) {
        return stack[--pointer];
    } else {
        printf("Błąd: za mała liczba argumentów\n");
        BEGIN(RESET);
    }
}

int main() {
    yylex();
}
