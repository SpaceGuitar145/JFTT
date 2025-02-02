%{
#include "AstNode.hpp"

ProgramNode* root;
void yyerror(const char* s);
extern int yylineno;
extern char* yytext;
extern int yylex();

void yyerror(const char *s) {
    std::cerr << "Error at line " << yylineno << ": " << s << std::endl;
    if (yytext && *yytext) {
        std::cerr << "Failed token: " << yytext << std::endl;
    }
}
%}

%debug
%code requires {
    #include <iostream>
    #include <string>
    #include "AstNode.hpp"
}

%union {
    std::string* strval;
    long long num;
    ProgramNode* program_node;
    ProceduresNode* procedures_node;
    MainNode* main_node;
    CommandsNode* commands_node;
    CommandNode* command_node;
    ProcedureHeadNode* proc_head_node;
    ProcedureCallNode* proc_call_node;
    DeclarationsNode* declarations_node;
    ArgumentsDeclarationNode* args_decl_node;
    ProcedureCallArguments* args_node;
    ExpressionNode* expression_node;
    ConditionNode* condition_node;
    IdentifierNode* identifier_node;
}

%token <strval> pidentifier
%token <num> num
%token PROGRAM PROCEDURE IS PROGRAM_BEGIN END
%token ASSIGN T
%token IF THEN ELSE ENDIF
%token WHILE DO ENDWHILE
%token REPEAT UNTIL
%token FOR FROM TO DOWNTO ENDFOR
%token WRITE READ
%token NEQ GEQ LEQ

%type <program_node> program_all
%type <procedures_node> procedures
%type <main_node> main
%type <commands_node> commands
%type <command_node> command
%type <proc_head_node> proc_head
%type <proc_call_node> proc_call
%type <declarations_node> declarations
%type <args_decl_node> args_decl
%type <args_node> args
%type <expression_node> value expression
%type <condition_node> condition
%type <identifier_node> identifier

%start program_all

%%

program_all:
    procedures main
    {
        root = new ProgramNode();
        root->addProcedures($1);
        root->addMain($2);
    }
;

procedures:
    procedures PROCEDURE proc_head IS declarations PROGRAM_BEGIN commands END
    {
        $1->addProcedure(new ProcedureNode($3, $5, $7));
        $$ = $1;
    }
    | procedures PROCEDURE proc_head IS PROGRAM_BEGIN commands END
    {
        $1->addProcedure(new ProcedureNode($3, $6));
        $$ = $1;
    }
    | 
    {
        $$ = new ProceduresNode();
    }
;

main:
    PROGRAM IS declarations PROGRAM_BEGIN commands END
    {
        $$ = new MainNode($3, $5);
    }
    | PROGRAM IS PROGRAM_BEGIN commands END
    {
        $$ = new MainNode($4);
    }
;

commands:
    commands command 
    {
        $1->addCommand($2);
        $$ = $1;
    }
    | command 
    {
        $$ = new CommandsNode();
        $$->addCommand($1);
    }
;

command:
    identifier ASSIGN expression ';'
    {
        $$ = new AssignNode($1, $3);
        $$->setLineNumber(yylineno);
    }
    | IF condition THEN commands ELSE commands ENDIF
    {
        $$ = new IfNode($2, $4, $6);
        $$->setLineNumber(yylineno);
    }
    | IF condition THEN commands ENDIF
    {
        $$ = new IfNode($2, $4);
        $$->setLineNumber(yylineno);
    }
    | WHILE condition DO commands ENDWHILE
    {
        $$ = new WhileNode($2, $4);
        $$->setLineNumber(yylineno);
    }
    | REPEAT commands UNTIL condition ';'
    {
        $$ = new RepeatUntilNode($4, $2); 
        $$->setLineNumber(yylineno);
    }
    | FOR pidentifier FROM value TO value DO commands ENDFOR 
    {
        $$ = new ForToNode($2, $4, $6, $8);
        $$->setLineNumber(yylineno);
    }
    | FOR pidentifier FROM value DOWNTO value DO commands ENDFOR
    {
        $$ = new ForDownToNode($2, $4, $6, $8);
        $$->setLineNumber(yylineno);
    }
    | proc_call ';'
    {
        $$ = $1;
        $$->setLineNumber(yylineno);
    }
    | READ identifier ';'
    {
        $$ = new ReadNode($2);
        $$->setLineNumber(yylineno);
    }
    | WRITE value ';'
    {
        $$ = new WriteNode($2);
        $$->setLineNumber(yylineno);
    }
;

proc_head:
    pidentifier '(' args_decl ')'
    {
        $$ = new ProcedureHeadNode($1, $3);
        $$->setLineNumber(yylineno);
    }
;

proc_call:
    pidentifier '(' args ')'
    {
        $$ = new ProcedureCallNode($1, $3);
        $$->setLineNumber(yylineno);
    }
;

declarations:
    declarations ',' pidentifier 
    {
        $1->setLineNumber(yylineno-1);
        $1->addVariableDeclaration($3);
        $$ = $1;
    }
    | declarations ',' pidentifier '[' num ':' num ']' 
    {
        $1->addArrayDeclaration($3, $5, $7);
        $$ = $1;
    }
    | pidentifier 
    {
        $$ = new DeclarationsNode();
        $$->setLineNumber(yylineno-1);
        $$->addVariableDeclaration($1);
    }
    | pidentifier '[' num ':' num ']' 
    {
        $$ = new DeclarationsNode();
        $$->addArrayDeclaration($1, $3, $5);
    }
;

args_decl:
    args_decl ',' pidentifier
    {
        $1->addVariableArgument($3);
        $$ = $1;
    }
    | args_decl ',' T pidentifier
    {
        $1->addArrayArgument($4);
        $$ = $1;
    }
    | pidentifier
    {
        $$ = new ArgumentsDeclarationNode();
        $$->addVariableArgument($1);
    }
    | T pidentifier
    {
        $$ = new ArgumentsDeclarationNode();
        $$->addArrayArgument($2);
    }
;

args:
    args ',' pidentifier 
    {
        $1->addArgument($3);
        $$->setLineNumber(yylineno);
        $$ = $1;
    }
    | pidentifier
    {
        $$ = new ProcedureCallArguments();
        $$->setLineNumber(yylineno);
        $$->addArgument($1);
    }
;

expression: 
    value
    {
        $$ = $1;
        $$->setLineNumber(yylineno);
    }
    | value '+' value
    {
        $$ = new BinaryExpressionNode($1,$3,"+");
        $$->setLineNumber(yylineno);
    }
    | value '-' value
    {
        $$ = new BinaryExpressionNode($1,$3,"-");
        $$->setLineNumber(yylineno);
    }
    | value '*' value
    {
        $$ = new BinaryExpressionNode($1,$3,"*");
        $$->setLineNumber(yylineno);
    }
    | value '/' value
    {
        $$ = new BinaryExpressionNode($1,$3,"/");
        $$->setLineNumber(yylineno);
    }
    | value '%' value
    {
        $$ = new BinaryExpressionNode($1,$3,"%");
        $$->setLineNumber(yylineno);
    }
;

condition: 
    value '=' value
    {
        $$ = new ConditionNode($1, $3, "=");
        $$->setLineNumber(yylineno);
    }
    | value NEQ value
    {
        $$ = new ConditionNode($1, $3, "!=");
        $$->setLineNumber(yylineno);
    }
    | value '>' value
    {
        $$ = new ConditionNode($1, $3, ">");
        $$->setLineNumber(yylineno);
    }
    | value '<' value
    {
        $$ = new ConditionNode($1, $3, "<");
        $$->setLineNumber(yylineno);
    }
    | value GEQ value
    {
        $$ = new ConditionNode($1, $3, ">=");
        $$->setLineNumber(yylineno);
    }
    | value LEQ value
    {
        $$ = new ConditionNode($1, $3, "<=");
        $$->setLineNumber(yylineno);
    }
;

value:
    num 
    {
        $$ = new ValueNode($1);
    }
    | identifier
    {
        $$ = $1;
    }
;

identifier:
    pidentifier
    {
        $$ = new IdentifierNode($1);
    }
    | pidentifier '[' pidentifier ']'
    {
        $$ = new IdentifierNode($1, new IdentifierNode($3));
    }
    | pidentifier '[' num ']'
    {
        $$ = new IdentifierNode($1, new ValueNode($3));
    }
;

%%