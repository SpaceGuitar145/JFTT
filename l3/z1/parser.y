%{
#include <stdio.h>
#include <stdlib.h>

#define MODULO 1234577

void yyerror(const char *s);
int yylex(void);

int mod(int x) {
    return ((x % MODULO) + MODULO) % MODULO;
}

int mod_multiply(int a, int b) {
    int result = 0;
    a = mod(a);
    while (b > 0) {
        if (b % 2 == 1) {
            result = mod(result + a);
        }
        a = mod(a * 2);
        b = b / 2;
    }
    return result;
}

int mod_pow(int base, int exponent) {
    int result = 1;
    while (exponent--) {
        result = mod_multiply(result, base);
    }
    return result;
}

int mult_inverse(int b, int m) {
    int x0 = 1, x1 = 0, m0 = m, t, q;
    while (b > 1) {
        q = b / m;
        t = m;
        m = b % m, b = t;
        t = x1;
        x1 = x0 - q * x1;
        x0 = t;
    }
    if (x0 < 0) {
        x0 += m0;
    }
    return x0;
}

int mod_div(int a, int b) {
    int b_inv = mult_inverse(b, MODULO);
    return mod_multiply(a, b_inv);
}

%}

%union {
    int num;
}

%token <num> NUMBER

%left '+' '-'
%left '*' '/'
%right '^'
%right UMINUS

%type <num> expr

%%

program:
    program expr '\n' { printf("Wynik: %d\n", $2); }
  | program '\n'
  | 
  ;

expr:
    expr '+' expr { $$ = mod($1 + $3); }
  | expr '-' expr { $$ = mod($1 - $3); }
  | expr '*' expr { $$ = mod_multiply($1, $3); }
  | expr '/' expr { 
        if ($3 != 0) 
            $$ = mod_div($1, $3); 
        else {
            printf("Błąd: dzielenie przez zero\n");
            $$ = 0;
        }
    }
  | expr '^' expr { $$ = mod_pow($1, $3); }
  | '-' expr %prec UMINUS { $$ = mod(-$2); }
  | '(' expr ')' { $$ = $2; }
  | NUMBER { $$ = mod($1); }
  ;

%%

int main(void) {
    while (1) yyparse();
}

void yyerror(const char *s) {
    fprintf(stderr, "Błąd: %s\n", s);
}
