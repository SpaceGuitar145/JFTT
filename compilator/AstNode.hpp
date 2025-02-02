#ifndef ASTNODE_HPP
#define ASTNODE_HPP

#include <iostream>
#include <vector>
#include <string>

class AstNode {
protected:
    int lineNumber;

public:
    AstNode() : lineNumber(0) {}
    virtual ~AstNode() = default;
    void setLineNumber(int line) { lineNumber = line; }
    int getLineNumber() const { return lineNumber; }
    virtual void print(int indent = 0) const = 0;

protected:
    void printIndent(int indent) const {
        std::cout << std::string(indent, ' ');
    }
};

class ExpressionNode : public AstNode {
public:
    ~ExpressionNode() override = default;
};

class ValueNode : public ExpressionNode {
public:
    long long value;

    explicit ValueNode(long long val) : value(val) {}

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Value: " << value << std::endl;
    }
};

class IdentifierNode : public ExpressionNode {
public:
    std::string* name;
    ExpressionNode* index = nullptr;
    long long start = 0, end = 0;
    bool isArrayRange = false;

    explicit IdentifierNode(std::string* varName)
        : name(varName) {}

    IdentifierNode(std::string* varName, ExpressionNode* idx)
        : name(varName), index(idx), isArrayRange(false) {}

    IdentifierNode(std::string* varName, long long startIdx, long long endIdx)
        : name(varName), index(nullptr), start(startIdx), end(endIdx), isArrayRange(true) {}

    ~IdentifierNode() {
        delete name;
        delete index;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Identifier: " << *name;
        if (isArrayRange) {
            std::cout << " [" << start << ":" << end << "]";
        } else if (index) {
            std::cout << " [";
            index->print(0);
            std::cout << "]";
        }
        std::cout << std::endl;
    }
};

class ArgumentNode : public AstNode {
public:
    std::string* argumentName;
    bool isArray = false;

    ~ArgumentNode() {
        delete argumentName;
    }

    explicit ArgumentNode(std::string* varName)
        : argumentName(varName) {}

    ArgumentNode(std::string* varName, bool isArr)
        : argumentName(varName), isArray(isArr) {}

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Argument: " << *argumentName;
        if (isArray) {
            std::cout << " T";
        }
        std::cout << std::endl;
    }
};

class ArgumentsDeclarationNode : public AstNode {
public:
    std::vector<ArgumentNode*> args;

    ~ArgumentsDeclarationNode() {
        for (auto arg : args) {
            delete arg;
        }
    }

    void addVariableArgument(std::string* pidentifier) {
        args.push_back(new ArgumentNode(pidentifier));
    }

    void addArrayArgument(std::string* pidentifier) {
        args.push_back(new ArgumentNode(pidentifier, true));
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Arguments:" << std::endl;
        for (const auto& arg : args) {
            arg->print(indent + 2);
        }
    }
};

class ProcedureHeadNode : public AstNode {
public:
    std::string* procedureName;
    ArgumentsDeclarationNode* argumentsDeclaration;

    ~ProcedureHeadNode() {
        delete procedureName;
        delete argumentsDeclaration;
    }

    explicit ProcedureHeadNode(std::string* pidentifier, ArgumentsDeclarationNode* argsDecl)
        : procedureName(pidentifier), argumentsDeclaration(argsDecl) {}

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Procedure Head: " << *procedureName << std::endl;

        if (argumentsDeclaration) {
            printIndent(indent + 2);
            std::cout << "Arguments Declaration:" << std::endl;
            argumentsDeclaration->print(indent + 4);
        }
    }
};

class ProcedureCallArguments : public AstNode {
public:
    std::vector<ExpressionNode*> arguments;

    ~ProcedureCallArguments() {
        for (auto arg : arguments) {
            delete arg;
        }
    }

    void addArgument(std::string* pidentifier) {
        arguments.push_back(new IdentifierNode(pidentifier));
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Procedure Call Arguments:" << std::endl;

        for (const auto& arg : arguments) {
            if (arg) {
                arg->print(indent + 2);
            }
        }
    }
};

class BinaryExpressionNode : public ExpressionNode {
public:
    ExpressionNode* left;
    ExpressionNode* right;
    std::string operation;

    explicit BinaryExpressionNode(ExpressionNode* lhs, ExpressionNode* rhs, const std::string& op)
        : left(lhs), right(rhs), operation(op) {}

    ~BinaryExpressionNode() {
        delete left;
        delete right;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Binary Expression: " << operation << std::endl;
        printIndent(indent + 2);
        std::cout << "Left:" << std::endl;
        left->print(indent + 4);
        printIndent(indent + 2);
        std::cout << "Right:" << std::endl;
        right->print(indent + 4);
    }
};

class ConditionNode : public AstNode {
public:
    ExpressionNode* leftValue;
    ExpressionNode* rightValue;
    std::string operation;

    explicit ConditionNode(ExpressionNode* leftVal, ExpressionNode* rightVal, const std::string& opr)
        : leftValue(leftVal), rightValue(rightVal), operation(opr) {}

    ~ConditionNode() {
        delete leftValue;
        delete rightValue;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Left:" << std::endl;
        if (leftValue) {
            leftValue->print(indent + 2);
        }

        printIndent(indent);
        std::cout << "Right:" << std::endl;
        if (rightValue) {
            rightValue->print(indent + 2);
        }

        printIndent(indent);
        std::cout << "Operation: " << operation << std::endl;
    }
};

class DeclarationsNode : public AstNode {
public:
    std::vector<IdentifierNode*> variables;

    ~DeclarationsNode() {
        for (auto var : variables) {
            delete var;
        }
    }

    void addVariableDeclaration(std::string* name) {
        variables.push_back(new IdentifierNode(name));
    }

    void addArrayDeclaration(std::string* name, long long start, long long end) {
        variables.push_back(new IdentifierNode(name, start, end));
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Declarations:" << std::endl;
        for (const auto& var : variables) {
            var->print(indent + 2);
        }
    }
};

class CommandNode : public AstNode {};

class CommandsNode : public AstNode {
public:
    std::vector<CommandNode*> commands;

    ~CommandsNode() {
        for (auto cmd : commands) {
            delete cmd;
        }
    }

    void addCommand(CommandNode* command) {
        commands.push_back(command);
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Commands:" << std::endl;
        for (const auto& cmd : commands) {
            cmd->print(indent + 2);
        }
    }
};

class AssignNode : public CommandNode {
public:
    IdentifierNode* identifier;
    ExpressionNode* expression;

    explicit AssignNode(IdentifierNode* id, ExpressionNode* exp)
        : identifier(id), expression(exp) {}

    ~AssignNode() {
        delete identifier;
        delete expression;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "AssignNode:" << std::endl;

        printIndent(indent + 2);
        std::cout << "Identifier:" << std::endl;
        if (identifier) {
            identifier->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "Expression:" << std::endl;
        if (expression) {
            expression->print(indent + 4);
        }
    }
};

class IfNode : public CommandNode {
public:
    ConditionNode* condition;
    CommandsNode* thenCommands;
    CommandsNode* elseCommands;

    explicit IfNode(ConditionNode* cond, CommandsNode* thenComms)
        : condition(cond), thenCommands(thenComms), elseCommands(nullptr) {}

    IfNode(ConditionNode* cond, CommandsNode* thenComms, CommandsNode* elseComms)
        : condition(cond), thenCommands(thenComms), elseCommands(elseComms) {}

    ~IfNode() {
        delete condition;
        delete thenCommands;
        delete elseCommands;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "IfNode:" << std::endl;

        printIndent(indent + 2);
        std::cout << "Condition:" << std::endl;
        if (condition) {
            condition->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "Then:" << std::endl;
        if (thenCommands) {
            thenCommands->print(indent + 4);
        }

        if (elseCommands) {
            printIndent(indent + 2);
            std::cout << "Else:" << std::endl;
            elseCommands->print(indent + 4);
        }
    }
};

class WhileNode : public CommandNode {
public:
    ConditionNode* condition;
    CommandsNode* commands;

    explicit WhileNode(ConditionNode* cond, CommandsNode* comms)
        : condition(cond), commands(comms) {}

    ~WhileNode() {
        delete condition;
        delete commands;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "WhileNode:" << std::endl;

        printIndent(indent + 2);
        std::cout << "Condition:" << std::endl;
        if (condition) {
            condition->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "Do:" << std::endl;
        if (commands) {
            commands->print(indent + 4);
        }
    }
};

class RepeatUntilNode : public CommandNode {
public:
    ConditionNode* condition;
    CommandsNode* commands;

    explicit RepeatUntilNode(ConditionNode* cond, CommandsNode* comms)
        : condition(cond), commands(comms) {}

    ~RepeatUntilNode() {
        delete condition;
        delete commands;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "RepeatUntilNode:" << std::endl;

        printIndent(indent + 2);
        std::cout << "Condition:" << std::endl;
        if (condition) {
            condition->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "Do:" << std::endl;
        if (commands) {
            commands->print(indent + 4);
        }
    }
};

class ForToNode : public CommandNode {
public:
    IdentifierNode* pidentifier;
    ExpressionNode* fromValue;
    ExpressionNode* toValue;
    CommandsNode* commands;

    ForToNode(std::string* pid, ExpressionNode* fromVal, ExpressionNode* toVal, CommandsNode* comms)
        : pidentifier(new IdentifierNode(pid)), fromValue(fromVal), toValue(toVal), commands(comms) {}

    ~ForToNode() {
        delete pidentifier;
        delete fromValue;
        delete toValue;
        delete commands;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "ForToNode:" << std::endl;

        printIndent(indent + 2);
        std::cout << "Variable: " << std::endl;
        if (pidentifier) {
            pidentifier->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "From:" << std::endl;
        if (fromValue) {
            fromValue->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "To:" << std::endl;
        if (toValue) {
            toValue->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "Do:" << std::endl;
        if (commands) {
            commands->print(indent + 4);
        }
    }
};

class ForDownToNode : public CommandNode {
public:
    IdentifierNode* pidentifier;
    ExpressionNode* fromValue;
    ExpressionNode* toValue;
    CommandsNode* commands;

    ForDownToNode(std::string* pid, ExpressionNode* fromVal, ExpressionNode* toVal, CommandsNode* comms)
        : pidentifier(new IdentifierNode(pid)), fromValue(fromVal), toValue(toVal), commands(comms) {}

    ~ForDownToNode() {
        delete pidentifier;
        delete fromValue;
        delete toValue;
        delete commands;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "ForToNode:" << std::endl;

        printIndent(indent + 2);
        std::cout << "Variable: " << std::endl;
        if (pidentifier) {
            pidentifier->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "From:" << std::endl;
        if (fromValue) {
            fromValue->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "Down To:" << std::endl;
        if (toValue) {
            toValue->print(indent + 4);
        }

        printIndent(indent + 2);
        std::cout << "Do:" << std::endl;
        if (commands) {
            commands->print(indent + 4);
        }
    }
};

class ProcedureCallNode : public CommandNode {
public:
    std::string* procedureName;
    ProcedureCallArguments* arguments;

    ~ProcedureCallNode() {
        delete procedureName;
        delete arguments;
    }

    explicit ProcedureCallNode(std::string* pidentifier, ProcedureCallArguments* args)
        : procedureName(pidentifier), arguments(args) {}

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Procedure Call: " << *procedureName << std::endl;

        if (arguments) {
            printIndent(indent + 2);
            std::cout << "Arguments:" << std::endl;
            arguments->print(indent + 4);
        }
    }
};

class WriteNode : public CommandNode {
public:
    ExpressionNode* value;

    explicit WriteNode(ExpressionNode* val) : value(val) {}

    ~WriteNode() {
        delete value;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Write:" << std::endl;
        value->print(indent + 2);
    }
};

class ReadNode : public CommandNode {
public:
    IdentifierNode* identifier;

    explicit ReadNode(IdentifierNode* val) : identifier(val) {}

    ~ReadNode() {
        delete identifier;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Read:" << std::endl;
        identifier->print(indent + 2);
    }
};

class MainNode : public AstNode {
public:
    DeclarationsNode* declarations;
    CommandsNode* commands;

    explicit MainNode(DeclarationsNode* decl, CommandsNode* cmds) 
        : declarations(decl), commands(cmds) {}

    MainNode(CommandsNode* cmds) 
        : declarations(nullptr), commands(cmds) {}

    ~MainNode() {
        delete declarations;
        delete commands;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Main:" << std::endl;
        if (declarations) {
            declarations->print(indent + 2);
        }
        if (commands) {
            commands->print(indent + 2);
        }
    }
};

class ProcedureNode : public AstNode {
public:
    ProcedureHeadNode* arguments = nullptr;
    DeclarationsNode* declarations = nullptr;
    CommandsNode* commands = nullptr;

    ~ProcedureNode() {
        delete arguments;
        delete declarations;
        delete commands;
    }

    explicit ProcedureNode(ProcedureHeadNode* args, CommandsNode* comms)
        : arguments(args), commands(comms) {}

    ProcedureNode(ProcedureHeadNode* args, DeclarationsNode* decls, CommandsNode* comms = nullptr)
        : arguments(args), declarations(decls), commands(comms) {}

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Procedure:" << std::endl;

        printIndent(indent + 2);
        std::cout << "Arguments:" << std::endl;
        if (arguments) {
            arguments->print(indent + 4);
        }

        if (declarations) {
            printIndent(indent + 2);
            std::cout << "Declarations:" << std::endl;
            declarations->print(indent + 4);
        }

        if (commands) {
            printIndent(indent + 2);
            std::cout << "Commands:" << std::endl;
            commands->print(indent + 4);
        }
    }
};

class ProceduresNode : public AstNode {
public:
    std::vector<ProcedureNode*> procedures;

    ~ProceduresNode() {
        for (auto procedure : procedures) {
            delete procedure;
        }
    }

    void addProcedure(ProcedureNode* procedure) {
        procedures.push_back(procedure);
    }

    void print(int indent = 0) const override {
        for (const auto& procedure : procedures) {
            procedure->print(indent + 2);
        }
    }
};

class ProgramNode : public AstNode {
public:
    MainNode* main;
    ProceduresNode* procedures;

    void addProcedures(ProceduresNode* procs) {
        procedures = procs;
    }

    void addMain(MainNode* mainNode) {
        main = mainNode;
    }

    ~ProgramNode() {
        delete main;
        delete procedures;
    }

    void print(int indent = 0) const override {
        printIndent(indent);
        std::cout << "Program:" << std::endl;

        if (procedures) {
            printIndent(indent + 2);
            std::cout << "Procedures:" << std::endl;
            procedures->print(indent + 2);
        }

        if (main) {
            printIndent(indent + 2);
            std::cout << "Main:" << std::endl;
            main->print(indent + 4);
        }
    }
};

#endif // ASTNODE_HPP
