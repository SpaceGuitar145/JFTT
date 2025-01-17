#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>

#include "AstNode.hpp"

class Instruction
{
public:
    std::string operation;
    long long argument;
    bool hasArgument;

    Instruction(const std::string &op, long long arg = 0, bool hasArg = false)
        : operation(op), argument(arg), hasArgument(hasArg) {}

    void print() const
    {
        if (hasArgument)
        {
            std::cout << operation << " " << argument << std::endl;
        }
        else
        {
            std::cout << operation << std::endl;
        }
    }
};

class CodeGenerator
{
private:
    std::vector<Instruction> instructions;
    std::unordered_map<std::string, long long> variableMemoryMap;
    long long memoryPointer = 1;

public:
    void generateProgram(ProgramNode *programNode)
    {
        if (!programNode)
        {
            throw std::runtime_error("Cannot generate code for null ProgramNode.");
        }

        if (programNode->procedures)
        {
            generateProcedures(programNode->procedures);
        }

        if (programNode->main)
        {
            generateMain(programNode->main);
        }

        instructions.emplace_back("HALT");
    }

    void generateProcedures(ProceduresNode *proceduresNode)
    {
        for (const auto &procedure : proceduresNode->procedures)
        {
            generateProcedure(procedure);
        }
    }

    void generateProcedure(ProcedureNode *procedureNode)
    {
        if (!procedureNode || !procedureNode->commands)
            return;

        generateDeclarations(procedureNode->declarations);

        generateCommands(procedureNode->commands);
    }

    void generateMain(MainNode *mainNode)
    {
        if (mainNode->declarations)
        {
            generateDeclarations(mainNode->declarations);
        }
        if (mainNode->commands)
        {
            generateCommands(mainNode->commands);
        }
    }

    void generateDeclarations(DeclarationsNode *declarationsNode)
    {
        for (const auto &var : declarationsNode->variables)
        {
            if (!var->isArrayRange)
            {
                allocateVariable(*var->name);
            }
        }
    }

    void generateCommands(CommandsNode *commandsNode)
    {
        for (const auto &command : commandsNode->commands)
        {
            if (auto readNode = dynamic_cast<ReadNode *>(command))
            {
                generateReadCommand(readNode);
            }
            else if (auto writeNode = dynamic_cast<WriteNode *>(command))
            {
                generateWriteCommand(writeNode);
            }
            else if (auto assignNode = dynamic_cast<AssignNode *>(command))
            {
                generateAssignCommand(assignNode);
            }
            else
            {
                throw std::runtime_error("Unsupported command type in CommandsNode.");
            }
        }
    }

    void generateReadCommand(ReadNode *readNode)
    {
        if (!readNode || !readNode->identifier)
            return;

        auto *identifier = readNode->identifier;
        long long address = getVariableMemoryAddress(*identifier->name);
        instructions.emplace_back("GET", address, true);
    }

    void generateWriteCommand(WriteNode *writeNode)
    {
        if (!writeNode || !writeNode->value)
            return;

        if (auto variable = dynamic_cast<IdentifierNode *>(writeNode->value))
        {
            long long address = getVariableMemoryAddress(*variable->name);
            instructions.emplace_back("PUT", address, true);
        }
        else if (auto valueNode = dynamic_cast<ValueNode *>(writeNode->value))
        {
            instructions.emplace_back("SET", valueNode->value, true);
            instructions.emplace_back("PUT", 0, true);
        }
    }

    void generateAssignCommand(AssignNode *assignNode)
    {
        if (!assignNode || !assignNode->identifier || !assignNode->expression)
            return;

        generateExpression(assignNode->expression);
        long long address = getVariableMemoryAddress(*assignNode->identifier->name);
        instructions.emplace_back("STORE", address, true);
    }

    void generateExpression(ExpressionNode *expression)
    {
        if (auto valueNode = dynamic_cast<ValueNode *>(expression))
        {
            instructions.emplace_back("SET", valueNode->value, true);
        }
        else if (auto variable = dynamic_cast<IdentifierNode *>(expression))
        {
            long long address = getVariableMemoryAddress(*variable->name);
            instructions.emplace_back("LOAD", address, true);
        }
        else if (auto binaryExpr = dynamic_cast<BinaryExpressionNode *>(expression))
        {
            generateExpression(binaryExpr->left);
            instructions.emplace_back("STORE", memoryPointer, true);
            long long leftTemp = memoryPointer++;
            generateExpression(binaryExpr->right);
            instructions.emplace_back("STORE", memoryPointer, true);
            long long rightTemp = memoryPointer++;

            if (binaryExpr->operation == "+")
            {
                instructions.emplace_back("ADD", leftTemp, true);
            }
            else if (binaryExpr->operation == "-")
            {
                instructions.emplace_back("SUB", leftTemp, true);
            }
            else if (binaryExpr->operation == "*")
            {
                instructions.emplace_back("SET", 0, true);
                instructions.emplace_back("STORE", memoryPointer, true);
                long long resultTemp = memoryPointer++;

                instructions.emplace_back("LOAD", rightTemp, true);

                int jumpForwardOffset = 8;
                instructions.emplace_back("JZERO", jumpForwardOffset, true);

                instructions.emplace_back("LOAD", resultTemp, true);
                instructions.emplace_back("ADD", leftTemp, true);
                instructions.emplace_back("STORE", resultTemp, true);

                instructions.emplace_back("LOAD", rightTemp, true);
                instructions.emplace_back("SUB", 1, true);
                instructions.emplace_back("STORE", rightTemp, true);

                int jumpBackOffset = -(instructions.size() - (instructions.size() - jumpForwardOffset) + 1);
                instructions.emplace_back("JUMP", jumpBackOffset, true);

                instructions.emplace_back("LOAD", resultTemp, true);
            }
            else if (binaryExpr->operation == "/")
            {
                if (auto valueNode = dynamic_cast<ValueNode *>(binaryExpr->right))
                {
                    if (valueNode->value == 0)
                    {
                        throw std::runtime_error("Division by zero!");
                    }
                }

                instructions.emplace_back("LOAD", rightTemp, true);
                instructions.emplace_back("JZERO", 12, true);

                instructions.emplace_back("SET", 0, true);
                instructions.emplace_back("STORE", memoryPointer, true);
                long long resultTemp = memoryPointer++;

                instructions.emplace_back("LOAD", leftTemp, true);
                instructions.emplace_back("SUB", rightTemp, true);
                instructions.emplace_back("JNEG", 7, true);

                instructions.emplace_back("STORE", leftTemp, true);

                instructions.emplace_back("LOAD", resultTemp, true);
                instructions.emplace_back("SET", 1, true);
                instructions.emplace_back("ADD", resultTemp, true);
                instructions.emplace_back("STORE", resultTemp, true);

                instructions.emplace_back("JUMP", -8, true);

                instructions.emplace_back("LOAD", resultTemp, true);
                memoryPointer -= 3;
            }
        }
    }

    void allocateVariable(const std::string &name)
    {
        if (variableMemoryMap.count(name))
        {
            throw std::runtime_error("Variable already declared: " + name);
        }
        variableMemoryMap[name] = memoryPointer++;
    }

    long long getVariableMemoryAddress(const std::string &name) const
    {
        auto it = variableMemoryMap.find(name);
        if (it == variableMemoryMap.end())
        {
            throw std::runtime_error("Undeclared variable: " + name);
        }
        return it->second;
    }

    void printInstructions() const
    {
        for (const auto &instr : instructions)
        {
            instr.print();
        }
    }
};