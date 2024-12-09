from sly import Lexer
from sly import Parser

MODULO = 1234577


def mod(x):
    return ((x % MODULO) + MODULO) % MODULO


def mod_multiply(a, b):
    result = 0
    a = mod(a)
    while b > 0:
        if b % 2 == 1:
            result = mod(result + a)
        a = mod(a * 2)
        b //= 2
    return result


def mod_pow(base, exponent):
    result = 1
    while exponent > 0:
        if exponent % 2 == 1:
            result = mod_multiply(result, base)
        base = mod_multiply(base, base)
        exponent //= 2
    return result


def mult_inverse(b, m):
    x0, x1, m0 = 1, 0, m
    while b > 1:
        q = b // m
        b, m = m, b % m
        x0, x1 = x1, x0 - q * x1
    if x0 < 0:
        x0 += m0
    return x0


def mod_div(a, b):
    b_inv = mult_inverse(b, MODULO)
    return mod_multiply(a, b_inv)

class CalcLexer(Lexer):
    tokens = { NUMBER }
    literals = { '+', '-', '*', '/', '^', '(', ')' }

    ignore = ' \t'

    @_(r'\d+')
    def NUMBER(self, t):
        t.value = int(t.value)
        return t

    @_(r'\#.*\\$')
    def ignore_comment_long(self, t):
        self.begin(CommentLexer)
    
    @_(r'\#.*')
    def comment(self, t):
        pass

    def error(self, t):
        print(f"Błąd: zły symbol '{t.value[0]}'")
        self.index += 1


class CommentLexer(Lexer):
    tokens = {}
    
    @_(r'[^ \t]+$')
    def end_of_input(self, t):
        self.begin(CalcLexer)


class CalcParser(Parser):
    tokens = CalcLexer.tokens

    precedence = (
        ('left', '+', '-'),
        ('left', '*', '/'),
        ('right', '^'),
        ('right', 'UMINUS'),
    )

    @_('expr')
    def program(self, p):
        if (p.expr):
            print(f"Wynik: {p.expr}")
        return p.expr
    
    @_('')
    def program(self, p):
        return None

    @_('expr "+" expr')
    def expr(self, p):
        return mod(p.expr0 + p.expr1)

    @_('expr "-" expr')
    def expr(self, p):
        return mod(p.expr0 - p.expr1)

    @_('expr "*" expr')
    def expr(self, p):
        return mod_multiply(p.expr0, p.expr1)

    @_('expr "/" expr')
    def expr(self, p):
        if p.expr1 == 0:
            print("Błąd: dzielienie przez zero")
            return None
        return mod_div(p.expr0, p.expr1)

    @_('expr "^" expr')
    def expr(self, p):
        return mod_pow(p.expr0, p.expr1)

    @_('"-" expr %prec UMINUS')
    def expr(self, p):
        return mod(-p.expr)

    @_('"(" expr ")"')
    def expr(self, p):
        return p.expr

    @_('NUMBER')
    def expr(self, p):
        return mod(p.NUMBER)

    def error(self, p):
        if p:
            print(f"Błąd w {p.value!r}")
        else:
            print("Błąd")

if __name__ == '__main__':
    lexer = CalcLexer()
    parser = CalcParser()

    while True:
        try:
            text = input('> ')
        except EOFError:
            break
        if text:
            parser.parse(lexer.tokenize(text))
