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
    std::unordered_map<std::string, std::pair<long long, long long>> arrayMemoryMap;
    std::unordered_map<std::string, long long> iteratorMemoryMap;
    std::unordered_map<std::string, long long> procedureEntryPoints;
    std::unordered_map<std::string, std::unordered_map<std::string, long long>> procedureArguments;
    std::unordered_map<std::string, std::unordered_map<std::string, long long>> procedureVariables;
    std::unordered_map<std::string, std::unordered_map<std::string, std::pair<long long, long long>>> procedureArrays;
    std::unordered_map<std::string, std::unordered_map<std::string, long long>> procedureArgumentsArrays;
    std::vector<std::string> procedureCalls;
    std::unordered_map<std::string, std::unordered_map<long long, std::pair<std::string, bool>>> argumentsPlaces;
    long long memoryPointer = 1;
    long long maxMemoryPointer = memoryPointer;

    void generateArrayAccess(IdentifierNode *identifier)
    {
        generateExpression(identifier->index);
        long long baseAddress = getArrayElementAddress(*identifier->name);
        instructions.emplace_back("ADD", baseAddress, true);
        instructions.emplace_back("STORE", memoryPointer, true);
    }

    void loadArrayValue(long long addressLocation)
    {
        instructions.emplace_back("LOADI", addressLocation, true);
    }

    void storeArrayValue(long long addressLocation)
    {
        instructions.emplace_back("STOREI", addressLocation, true);
    }

    void handleArrayValues(bool isLeftArray, bool isRightArray, long long leftTemp, long long rightTemp, long long &leftValue, long long &rightValue)
    {
        leftValue = leftTemp;
        rightValue = rightTemp;

        if (isLeftArray)
        {
            loadArrayValue(leftTemp);
            instructions.emplace_back("STORE", memoryPointer, true);
            leftValue = memoryPointer++;
        }

        if (isRightArray)
        {
            loadArrayValue(rightTemp);
            instructions.emplace_back("STORE", memoryPointer, true);
            rightValue = memoryPointer++;
        }
    }

    void initializeResultAndSign(long long &resultTemp, long long &signTemp)
    {
        instructions.emplace_back("SET", 0, true);
        instructions.emplace_back("STORE", memoryPointer, true);
        resultTemp = memoryPointer++;

        instructions.emplace_back("SET", 1, true);
        instructions.emplace_back("STORE", memoryPointer, true);
        signTemp = memoryPointer++;
    }

    void handleOperandSign(long long value, long long signTemp, bool isFirst)
    {
        instructions.emplace_back("SET", 0, true);
        instructions.emplace_back("SUB", value, true);
        if (isFirst)
        {
            instructions.emplace_back("JPOS", 2, true);
            instructions.emplace_back("JUMP", 4, true);
            instructions.emplace_back("STORE", value, true);
            instructions.emplace_back("SET", -1, true);
            instructions.emplace_back("STORE", signTemp, true);
        }
        else
        {
            instructions.emplace_back("JPOS", 5, true);
            instructions.emplace_back("SET", 1, true);
            instructions.emplace_back("ADD", signTemp, true);
            instructions.emplace_back("STORE", signTemp, true);
            instructions.emplace_back("JUMP", 5, true);
            instructions.emplace_back("STORE", value, true);
            instructions.emplace_back("SET", -1, true);
            instructions.emplace_back("ADD", signTemp, true);
            instructions.emplace_back("STORE", signTemp, true);
        }
    }

    void performMultiplication(long long leftValue, long long rightValue, long long resultTemp)
    {
        instructions.emplace_back("LOAD", rightValue, true);
        instructions.emplace_back("JZERO", 8, true);
        instructions.emplace_back("LOAD", resultTemp, true);
        instructions.emplace_back("ADD", leftValue, true);
        instructions.emplace_back("STORE", resultTemp, true);
        instructions.emplace_back("SET", -1, true);
        instructions.emplace_back("ADD", rightValue, true);
        instructions.emplace_back("STORE", rightValue, true);
        instructions.emplace_back("JUMP", -7, true);
    }

    void applySign(long long resultTemp, long long signTemp)
    {
        instructions.emplace_back("LOAD", signTemp, true);
        instructions.emplace_back("JZERO", 3, true);
        instructions.emplace_back("LOAD", resultTemp, true);
        instructions.emplace_back("JUMP", 3, true);
        instructions.emplace_back("SET", 0, true);
        instructions.emplace_back("SUB", resultTemp, true);
    }

    void performDivision(long long leftValue, long long rightValue, long long resultTemp)
    {
        instructions.emplace_back("LOAD", leftValue, true);
        instructions.emplace_back("SUB", rightValue, true);
        instructions.emplace_back("JNEG", 7, true);
        instructions.emplace_back("STORE", leftValue, true);
        instructions.emplace_back("LOAD", resultTemp, true);
        instructions.emplace_back("SET", 1, true);
        instructions.emplace_back("ADD", resultTemp, true);
        instructions.emplace_back("STORE", resultTemp, true);
        instructions.emplace_back("JUMP", -8, true);
    }

    void allocateIterator(const std::string &name)
    {
        // if (iteratorMemoryMap.count(name) || variableMemoryMap.count(name))
        // {
        //     throw std::runtime_error("Iterator or variable already declared: " + name);
        // }
        iteratorMemoryMap[name] = memoryPointer++;
    }

    void deallocateIterator(const std::string &name)
    {
        if (iteratorMemoryMap.count(name))
        {
            maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);
            memoryPointer--;
            iteratorMemoryMap.erase(name);
        }
    }

    long long getIteratorAddress(const std::string &name) const
    {
        auto it = iteratorMemoryMap.find(name);
        if (it == iteratorMemoryMap.end())
        {
            throw std::runtime_error("Undeclared iterator: " + name);
        }
        return it->second;
    }

    void allocateProcedureArgument(const std::string &procedureName, const std::string &argumentName)
    {
        // std::cout << "add var argument" << std::endl;

        if (procedureArguments[procedureName].count(argumentName))
        {
            throw std::runtime_error("Argument already declared: " + argumentName);
        }

        long long orderIndex = argumentsPlaces[procedureName].size() + 1;
        argumentsPlaces[procedureName][orderIndex] = {argumentName, false};

        // std::cout << "Argument: " << argumentName << " " << orderIndex << std::endl;
        // std::cout << "Argument: " << argumentsPlaces[procedureName][orderIndex].first << " " << orderIndex << std::endl;

        // std::cout << "Memory Pointer: " << memoryPointer << std::endl;

        procedureArguments[procedureName][argumentName] = memoryPointer++;
    }

    void allocateProcedureArgumentArray(const std::string &procedureName, const std::string &argumentName)
    {
        if (procedureArgumentsArrays[procedureName].count(argumentName))
        {
            throw std::runtime_error("Argument already declared: " + argumentName);
        }

        long long orderIndex = argumentsPlaces[procedureName].size() + 1;
        argumentsPlaces[procedureName][orderIndex] = {argumentName, true};

        procedureArgumentsArrays[procedureName][argumentName] = memoryPointer++;
    }

    void allocateProcedureVariable(const std::string &procedureName, const std::string &variableName)
    {
        // Check if variable exists in procedure arguments
        if (procedureArguments[procedureName].count(variableName) ||
            procedureArgumentsArrays[procedureName].count(variableName))
        {
            throw std::runtime_error("Variable name conflicts with procedure argument: " + variableName);
        }

        if (procedureVariables[procedureName].count(variableName))
        {
            throw std::runtime_error("Variable already declared: " + variableName);
        }
        procedureVariables[procedureName][variableName] = memoryPointer++;

        std::cout << "Procedure Variable: " << procedureName << " " << variableName << procedureVariables[procedureName][variableName] << std::endl;
    }

    void allocateProcedureArray(const std::string &procedureName, const std::string &variableName, long long start, long long end)
    {
        // Check if array name exists in procedure arguments
        if (procedureArguments[procedureName].count(variableName) ||
            procedureArgumentsArrays[procedureName].count(variableName))
        {
            throw std::runtime_error("Array name conflicts with procedure argument: " + variableName);
        }

        if (arrayMemoryMap.count(procedureName))
        {
            throw std::runtime_error("Array already declared: " + procedureName);
        }
        if (start > end)
        {
            throw std::runtime_error("Invalid array range: start > end");
        }
        long long size = end - start + 1;
        instructions.emplace_back("SET", memoryPointer - start, true);
        memoryPointer += size;
        instructions.emplace_back("STORE", memoryPointer, true);
        procedureArrays[procedureName][variableName] = {memoryPointer, size};
        memoryPointer++;
    }

    void allocateReturnVariable(const std::string &procedureName)
    {
        if (procedureVariables[procedureName].count("return"))
        {
            throw std::runtime_error("Return variable already declared");
        }

        procedureVariables[procedureName]["return"] = memoryPointer++;
    }

    long long getProcedureArgumentAddress(const std::string &name) const
    {
        const std::string &currentProcedure = procedureCalls.back();
        auto procIt = procedureArguments.find(currentProcedure);
        if (procIt == procedureArguments.end())
        {
            throw std::runtime_error("Procedure not found: " + currentProcedure);
        }

        auto argIt = procIt->second.find(name);
        if (argIt == procIt->second.end())
        {
            throw std::runtime_error("Argument not found in procedure " + currentProcedure + ": " + name);
        }

        return argIt->second;
    }

    long long getProcedureArgumentsArrayElementAddress(const std::string &name) const
    {
        const std::string &currentProcedure = procedureCalls.back();
        auto procIt = procedureArgumentsArrays.find(currentProcedure);
        if (procIt == procedureArgumentsArrays.end())
        {
            throw std::runtime_error("Procedure not found: " + currentProcedure);
        }

        auto varIt = procIt->second.find(name);
        if (varIt == procIt->second.end())
        {
            throw std::runtime_error("Argument not found in procedure " + currentProcedure + ": " + name);
        }

        return varIt->second;
    }

    long long getProcedureVariableAddress(const std::string &name) const
    {
        if (procedureCalls.empty())
        {
            throw std::runtime_error("No active procedure context");
        }

        const std::string &currentProcedure = procedureCalls.back();
        auto procIt = procedureVariables.find(currentProcedure);
        if (procIt == procedureVariables.end())
        {
            throw std::runtime_error("Procedure not found: " + currentProcedure);
        }

        auto varIt = procIt->second.find(name);
        if (varIt == procIt->second.end())
        {
            throw std::runtime_error("Variable not found in procedure " + currentProcedure + ": " + name);
        }

        return varIt->second;
    }

    long long getProcedureIdentifierAddress(const std::string &name) const
    {
        const std::string &currentProcedure = procedureCalls.back();

        // std::cout << "current context: " << currentProcedure << std::endl;

        // Check procedure arguments
        auto argIt = procedureArguments.find(currentProcedure);
        // if (argIt != procedureArguments.end())
        // {
        //     // Iterate through the inner map
        //     std::cout << "Arguments for procedure '" << currentProcedure << "':" << std::endl;
        //     for (const auto &[argName, argValue] : argIt->second)
        //     {
        //         std::cout << "  Argument: " << argName << ", Address: " << argValue << std::endl;
        //     }
        // }
        // else
        // {
        //     std::cout << "Procedure not found: " << currentProcedure << std::endl;
        // }
        if (argIt != procedureArguments.end())
        {

            auto varIt = argIt->second.find(name);
            if (varIt != argIt->second.end())
            {
                std::cout << "argument: " << varIt->first << " " << varIt->second << std::endl;
                return 1; // Argument
            }
        }

        // Check procedure variables
        auto varIt = procedureVariables.find(currentProcedure);
        // if (varIt != procedureVariables.end())
        // {
        //     // Iterate through the inner map
        //     std::cout << "Variables for procedure '" << currentProcedure << "':" << std::endl;
        //     for (const auto &[argName, argValue] : varIt->second)
        //     {
        //         std::cout << "  Variable: " << argName << ", Address: " << argValue << std::endl;
        //     }
        // }
        // else
        // {
        //     std::cout << "Procedure not found: " << currentProcedure << std::endl;
        // }
        if (varIt != procedureVariables.end())
        {
            auto localIt = varIt->second.find(name);
            if (localIt != varIt->second.end())
            {
                return 2; // Variable
            }
        }

        // Check procedure arrays
        auto arrIt = procedureArrays.find(currentProcedure);
        if (arrIt != procedureArrays.end())
        {
            auto localIt = arrIt->second.find(name);
            if (localIt != arrIt->second.end())
            {
                return 3; // Array
            }
        }

        // Check procedure argument arrays
        auto argArrIt = procedureArgumentsArrays.find(currentProcedure);
        if (argArrIt != procedureArgumentsArrays.end())
        {
            auto localIt = argArrIt->second.find(name);
            if (localIt != argArrIt->second.end())
            {
                return 4; // Argument Array
            }
        }

        return -1; // Not found
    }

    long long getIdentifierAddress(const std::string &name) const
    {
        auto it = arrayMemoryMap.find(name);

        if (it != arrayMemoryMap.end())
        {
            return 1;
        }

        return -1;
    }

    long long getProcedureArrayElementAddress(const std::string &name) const
    {
        const std::string &currentProcedure = procedureCalls.back();
        auto procIt = procedureArrays.find(currentProcedure);
        if (procIt == procedureArrays.end())
        {
            throw std::runtime_error("Procedure not found: " + currentProcedure);
        }

        auto arrIt = procIt->second.find(name);
        if (arrIt == procIt->second.end())
        {
            throw std::runtime_error("Array not found in procedure " + currentProcedure + ": " + name);
        }

        return arrIt->second.first;
    }

    long long getProcedureArgumentArrayElementAddress(const std::string &name) const
    {
        const std::string &currentProcedure = procedureCalls.back();
        auto procIt = procedureArgumentsArrays.find(currentProcedure);
        if (procIt == procedureArgumentsArrays.end())
        {
            throw std::runtime_error("Procedure not found: " + currentProcedure);
        }

        auto arrIt = procIt->second.find(name);
        if (arrIt == procIt->second.end())
        {
            throw std::runtime_error("Array not found in procedure " + currentProcedure + ": " + name);
        }

        return arrIt->second;
    }

    long long generateProcedureArrayAccess(IdentifierNode *identifier)
    {
        generateExpression(identifier->index);
        long long baseAddress = getProcedureArrayElementAddress(*identifier->name);
        instructions.emplace_back("ADD", baseAddress, true);
        instructions.emplace_back("STORE", memoryPointer, true);
        return memoryPointer;
    }

    long long generateProcedureArgumentsArrayAccess(IdentifierNode *identifier)
    {
        generateExpression(identifier->index);
        long long baseAddress = getProcedureArgumentArrayElementAddress(*identifier->name);
        instructions.emplace_back("ADDI", baseAddress, true);
        instructions.emplace_back("STORE", memoryPointer, true);
        return memoryPointer;
    }

public:
    void generateProgram(ProgramNode *programNode)
    {
        if (!programNode)
        {
            throw std::runtime_error("Cannot generate code for null ProgramNode.");
        }

        procedureCalls.emplace_back("main");

        long long jumpToMain;
        if (programNode->procedures)
        {
            instructions.emplace_back("JUMP", 0, true);
            jumpToMain = instructions.size() - 1;
            generateProcedures(programNode->procedures);
        }

        if (programNode->main)
        {
            memoryPointer = std::max(memoryPointer, maxMemoryPointer);
            if (programNode->procedures)
            {
                instructions[jumpToMain].argument = instructions.size() - jumpToMain;
            }
            generateMain(programNode->main);
            procedureCalls.pop_back();
        }

        instructions.emplace_back("HALT");
    }

    void generateProcedures(ProceduresNode *proceduresNode)
    {
        for (const auto &procedure : proceduresNode->procedures)
        {
            generateProcedure(procedure);
            // std::cout << "Max Memory Pointer: " << maxMemoryPointer << std::endl;
            memoryPointer = maxMemoryPointer >= memoryPointer ? maxMemoryPointer + 1 : memoryPointer;
            // std::cout << "Memory Pointer: " << memoryPointer << std::endl;
        }
    }

    void generateProcedure(ProcedureNode *procedureNode)
    {
        if (!procedureNode || !procedureNode->commands)
            return;

        procedureEntryPoints[*procedureNode->arguments->procedureName] = instructions.size();

        generateProcedureHead(procedureNode->arguments);

        // std::cout << "Memory Pointer: " << memoryPointer << std::endl;

        if (procedureNode->declarations)
        {
            generateProcedureDeclarations(procedureNode->arguments->procedureName, procedureNode->declarations);
        }

        allocateReturnVariable(*procedureNode->arguments->procedureName);

        procedureCalls.emplace_back(*procedureNode->arguments->procedureName);
        generateCommands(procedureNode->commands);
        procedureCalls.pop_back();
        // std::cout << "Memory Pointer: " << memoryPointer << std::endl;

        instructions.emplace_back("RTRN", procedureVariables[*procedureNode->arguments->procedureName]["return"], true);
    }

    void generateProcedureHead(ProcedureHeadNode *procedureHead)
    {
        if (!procedureHead)
            return;

        if (procedureHead->argumentsDeclaration)
        {
            generateArgumentsDeclaration(procedureHead->procedureName, procedureHead->argumentsDeclaration);
        }
    }

    void generateProcedureDeclarations(std::string *procedureName, DeclarationsNode *declarationsNode)
    {
        for (const auto &var : declarationsNode->variables)
        {
            if (var->isArrayRange)
            {
                allocateProcedureArray(*procedureName, *var->name, var->start, var->end);
            }
            else
            {
                std::cout << "Generating procedure variable: " << *procedureName << " " << *var->name << std::endl;
                allocateProcedureVariable(*procedureName, *var->name);
            }
        }
    }

    void generateArgumentsDeclaration(std::string *procedureName, ArgumentsDeclarationNode *argumentsDeclarationNode)
    {
        for (const auto &arg : argumentsDeclarationNode->args)
        {
            if (arg->isArray)
            {
                allocateProcedureArgumentArray(*procedureName, *arg->argumentName);
            }
            else
            {
                allocateProcedureArgument(*procedureName, *arg->argumentName);
            }
        }
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
            if (var->isArrayRange)
            {
                allocateArray(*var->name, var->start, var->end);
            }
            else
            {
                allocateVariable(*var->name);
            }
        }
    }

    void generateCommands(CommandsNode *commandsNode)
    {
        for (const auto &command : commandsNode->commands)
        {
            if (auto ifNode = dynamic_cast<IfNode *>(command))
            {
                generateIfCommand(ifNode);
            }
            else if (auto readNode = dynamic_cast<ReadNode *>(command))
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
            else if (auto repeatUntilNode = dynamic_cast<RepeatUntilNode *>(command))
            {
                generateRepeatUntilCommand(repeatUntilNode);
            }
            else if (auto whileNode = dynamic_cast<WhileNode *>(command))
            {
                generateWhileNode(whileNode);
            }
            else if (auto forToNode = dynamic_cast<ForToNode *>(command))
            {
                generateForToNode(forToNode);
            }
            else if (auto procedureCallNode = dynamic_cast<ProcedureCallNode *>(command))
            {
                generateProcedureCall(procedureCallNode);
            }
            else if (auto forDownToNode = dynamic_cast<ForDownToNode *>(command))
            {
                generateForDownToNode(forDownToNode);
            }
            else
            {
                throw std::runtime_error("Unsupported command type in CommandsNode.");
            }
        }
    }

    void generateProcedureCallArguments(const std::string &procedureName, ProcedureCallArguments *arguments)
    {
        std::cout << "Generating procedure call arguments for: " << procedureName << std::endl;
        if (arguments->arguments.size() != argumentsPlaces[procedureName].size())
        {
            throw std::runtime_error("Argument count mismatch for procedure " + procedureName + ": expected " + std::to_string(argumentsPlaces[procedureName].size()) + " but got " + std::to_string(arguments->arguments.size()));
        }
        for (size_t i = 1; i <= arguments->arguments.size(); ++i)
        {
            const auto &arg = arguments->arguments[i - 1];
            const auto &expectedArg = argumentsPlaces[procedureName][i];

            if (auto identifier = dynamic_cast<IdentifierNode *>(arg))
            {
                std::cout << "Argument: " << *identifier->name << " Expected: " << expectedArg.first << std::endl;
                std::cout << "Procedure current: " << procedureCalls.back() << std::endl;

                std::string procedureRemember = procedureCalls.back();
                procedureCalls.pop_back();
                auto sign = getProcedureIdentifierAddress(*identifier->name);
                std::cout << "Identifier sign: " << sign << std::endl;
                procedureCalls.emplace_back(procedureRemember);
                if (sign != -1)
                {
                    if (sign == 1)
                    {
                        if (expectedArg.second)
                        {
                            throw std::runtime_error("Argument type mismatch for procedure " + procedureName + ": expected " + (expectedArg.second ? "array" : "variable") + " but got " + (identifier->isArrayRange ? "array" : "variable"));
                        }
                        std::string procedureRemember = procedureCalls.back();
                        procedureCalls.pop_back();
                        long long address = getProcedureArgumentAddress(*identifier->name);
                        instructions.emplace_back("LOAD", address, true);
                        procedureCalls.emplace_back(procedureRemember);
                        long long argumentAddress = getProcedureArgumentAddress(expectedArg.first);
                        instructions.emplace_back("STORE", argumentAddress, true);
                        memoryPointer++;
                    }
                    else if (sign == 2)
                    {
                        if (expectedArg.second)
                        {
                            throw std::runtime_error("Argument type mismatch for procedure " + procedureName + ": expected " + (expectedArg.second ? "array" : "variable") + " but got " + (identifier->isArrayRange ? "array" : "variable"));
                        }
                        std::string procedureRemember = procedureCalls.back();
                        procedureCalls.pop_back();
                        long long address = getProcedureVariableAddress(*identifier->name);
                        instructions.emplace_back("SET", address, true);
                        procedureCalls.emplace_back(procedureRemember);
                        long long argumentAddress = getProcedureArgumentAddress(expectedArg.first);
                        instructions.emplace_back("STORE", argumentAddress, true);
                        memoryPointer++;
                    }
                    else if (sign == 3)
                    {
                        if (!expectedArg.second)
                        {
                            throw std::runtime_error("Argument type mismatch for procedure " + procedureName + ": expected " + (expectedArg.second ? "array" : "variable") + " but got " + (identifier->isArrayRange ? "variable" : "array"));
                        }
                        std::string procedureRemember = procedureCalls.back();
                        procedureCalls.pop_back();
                        long long address = getProcedureArrayElementAddress(*identifier->name);
                        instructions.emplace_back("SET", address, true);
                        procedureCalls.emplace_back(procedureRemember);
                        long long argumentAddress = getProcedureArgumentsArrayElementAddress(expectedArg.first);
                        instructions.emplace_back("STORE", argumentAddress, true);
                        memoryPointer++;
                    }
                    else if (sign == 4)
                    {
                        if (!expectedArg.second)
                        {
                            throw std::runtime_error("Argument type mismatch for procedure " + procedureName + ": expected " + (expectedArg.second ? "array" : "variable") + " but got " + (identifier->isArrayRange ? "array" : "variable"));
                        }
                        std::string procedureRemember = procedureCalls.back();
                        procedureCalls.pop_back();
                        long long address = getProcedureArgumentsArrayElementAddress(*identifier->name);
                        instructions.emplace_back("LOAD", address, true);
                        procedureCalls.emplace_back(procedureRemember);
                        long long argumentAddress = getProcedureArgumentsArrayElementAddress(expectedArg.first);
                        instructions.emplace_back("STORE", argumentAddress, true);
                        memoryPointer++;
                    }
                }
                else if (auto test = getIdentifierAddress(*identifier->name); test != -1)
                {
                    if (!expectedArg.second)
                    {
                        throw std::runtime_error("Argument type mismatch for procedure " + procedureName + ": expected " + (expectedArg.second ? "array" : "variable") + " but got " + (identifier->isArrayRange ? "variable" : "array"));
                    }

                    long long address = getArrayElementAddress(*identifier->name);
                    instructions.emplace_back("SET", address, true);
                    long long argumentAddress = getProcedureArgumentArrayElementAddress(expectedArg.first);
                    instructions.emplace_back("STORE", argumentAddress, true);
                    memoryPointer++;
                }
                else
                {
                    if (expectedArg.second)
                    {
                        throw std::runtime_error("Argument type mismatch for procedure " + procedureName + ": expected " + (expectedArg.second ? "array" : "variable") + " but got " + (identifier->isArrayRange ? "array" : "variable"));
                    }

                    long long address = getVariableMemoryAddress(*identifier->name);
                    instructions.emplace_back("SET", address, true);
                    long long argumentAddress = getProcedureArgumentAddress(expectedArg.first);
                    instructions.emplace_back("STORE", argumentAddress, true);
                    memoryPointer++;
                }
            }
        }
    }

    void generateProcedureCall(ProcedureCallNode *procedureCallNode)
    {
        if (!procedureCallNode)
            return;

        std::cout << "Generating procedure call for: " << *procedureCallNode->procedureName << std::endl;

        if (procedureCallNode->arguments)
        {
            if (procedureEntryPoints[*procedureCallNode->procedureName] == 0)
            {
                throw std::runtime_error("Procedure not found: " + *procedureCallNode->procedureName);
            }
            procedureCalls.emplace_back(*procedureCallNode->procedureName);
            generateProcedureCallArguments(*procedureCallNode->procedureName, procedureCallNode->arguments);
            instructions.emplace_back("SET", instructions.size() + 3, true);
            instructions.emplace_back("STORE", procedureVariables[*procedureCallNode->procedureName]["return"], true);
            procedureCalls.pop_back();
        }

        instructions.emplace_back("JUMP", procedureEntryPoints[*procedureCallNode->procedureName] - instructions.size(), true);
    }

    void generateRepeatUntilCommand(RepeatUntilNode *repeatUntilNode)
    {
        long long start = instructions.size();
        generateCommands(repeatUntilNode->commands);
        generateCondition(repeatUntilNode->condition);

        // std::cout << repeatUntilNode->condition->operation << std::endl;

        if (repeatUntilNode->condition->operation == "=")
        {
            instructions.emplace_back("JZERO", 2, true);
            instructions.emplace_back("JUMP", start - instructions.size(), true);
        }
        else if (repeatUntilNode->condition->operation == "!=")
        {
            instructions.emplace_back("JZERO", start - instructions.size(), true);
        }
        else if (repeatUntilNode->condition->operation == "<")
        {
            instructions.emplace_back("JPOS", 2, true);
            instructions.emplace_back("JUMP", start - instructions.size(), true);
        }
        else if (repeatUntilNode->condition->operation == ">")
        {
            instructions.emplace_back("JNEG", 2, true);
            instructions.emplace_back("JUMP", start - instructions.size(), true);
        }
        else if (repeatUntilNode->condition->operation == ">=")
        {
            instructions.emplace_back("JPOS", start - instructions.size(), true);
        }
        else if (repeatUntilNode->condition->operation == "<=")
        {
            instructions.emplace_back("JNEG", start - instructions.size(), true);
        }
    }

    void generateWhileNode(WhileNode *whileNode)
    {
        long long start = instructions.size();
        generateCondition(whileNode->condition);

        if (whileNode->condition->operation == "=")
        {
            instructions.emplace_back("JZERO", 2, true);

            instructions.emplace_back("JUMP", 0, true);
            long long skipDoBlock = instructions.size() - 1;

            generateCommands(whileNode->commands);
            instructions.emplace_back("JUMP", start - instructions.size(), true);

            instructions[skipDoBlock].argument = instructions.size() - skipDoBlock;
        }
        else if (whileNode->condition->operation == "!=")
        {
            instructions.emplace_back("JZERO", 2, true);
            long long skipDoBlock = instructions.size() - 1;

            generateCommands(whileNode->commands);

            instructions.emplace_back("JUMP", start - instructions.size(), true);

            instructions[skipDoBlock].argument = instructions.size() - skipDoBlock;
        }
        else if (whileNode->condition->operation == "<")
        {
            instructions.emplace_back("JPOS", 2, true);
            instructions.emplace_back("JUMP", 0, true);
            long long skipDoBlock = instructions.size() - 1;

            generateCommands(whileNode->commands);
            instructions.emplace_back("JUMP", start - instructions.size(), true);

            instructions[skipDoBlock].argument = instructions.size() - skipDoBlock;
        }
        else if (whileNode->condition->operation == ">")
        {
            instructions.emplace_back("JNEG", 2, true);
            instructions.emplace_back("JUMP", 0, true);
            long long skipDoBlock = instructions.size() - 1;

            generateCommands(whileNode->commands);
            instructions.emplace_back("JUMP", start - instructions.size(), true);

            instructions[skipDoBlock].argument = instructions.size() - skipDoBlock;
        }
        else if (whileNode->condition->operation == ">=")
        {
            instructions.emplace_back("JPOS", start - instructions.size(), true);
            long long skipDoBlock = instructions.size() - 1;

            generateCommands(whileNode->commands);

            instructions.emplace_back("JUMP", start - instructions.size(), true);

            instructions[skipDoBlock].argument = instructions.size() - skipDoBlock;
        }
        else if (whileNode->condition->operation == "<=")
        {
            instructions.emplace_back("JNEG", start - instructions.size(), true);
            long long skipDoBlock = instructions.size() - 1;

            generateCommands(whileNode->commands);

            instructions.emplace_back("JUMP", start - instructions.size(), true);

            instructions[skipDoBlock].argument = instructions.size() - skipDoBlock;
        }
    }

    void generateForToNode(ForToNode *forToNode)
    {
        allocateIterator(*forToNode->pidentifier->name);
        long long iterator = getIteratorAddress(*forToNode->pidentifier->name);

        if (auto fromValue = dynamic_cast<IdentifierNode *>(forToNode->fromValue))
        {
            auto it = iteratorMemoryMap.find(*fromValue->name);
            if (fromValue->index)
            {
                generateArrayAccess(fromValue);
                loadArrayValue(memoryPointer);
            }
            else if (it != iteratorMemoryMap.end())
            {
                long long address = getIteratorAddress(*fromValue->name);
                instructions.emplace_back("LOAD", address, true);
            }
            else
            {
                long long address = getVariableMemoryAddress(*fromValue->name);
                instructions.emplace_back("LOAD", address, true);
            }
        }
        else if (auto valueNode = dynamic_cast<ValueNode *>(forToNode->fromValue))
        {
            instructions.emplace_back("SET", valueNode->value, true);
        }

        instructions.emplace_back("STORE", iterator, true);

        if (auto toValue = dynamic_cast<IdentifierNode *>(forToNode->toValue))
        {
            auto it = iteratorMemoryMap.find(*toValue->name);
            if (toValue->index)
            {
                generateArrayAccess(toValue);
                loadArrayValue(memoryPointer);
            }
            else if (it != iteratorMemoryMap.end())
            {
                long long address = getIteratorAddress(*toValue->name);
                instructions.emplace_back("LOAD", address, true);
            }
            else
            {
                long long address = getVariableMemoryAddress(*toValue->name);
                instructions.emplace_back("LOAD", address, true);
            }
        }
        else if (auto valueNode = dynamic_cast<ValueNode *>(forToNode->toValue))
        {
            instructions.emplace_back("SET", valueNode->value, true);
        }

        instructions.emplace_back("STORE", memoryPointer, true);
        long long toValue = memoryPointer++;

        instructions.emplace_back("LOAD", iterator, true);
        long long loopStart = instructions.size() - 1;
        instructions.emplace_back("SUB", toValue, true);
        instructions.emplace_back("JPOS", 0, true);
        long long skipLoopJump = instructions.size() - 1;

        generateCommands(forToNode->commands);

        instructions.emplace_back("SET", 1, true);
        instructions.emplace_back("ADD", iterator, true);
        instructions.emplace_back("STORE", iterator, true);

        instructions.emplace_back("JUMP", loopStart - instructions.size(), true);

        long long loopEnd = instructions.size();
        instructions[skipLoopJump].argument = loopEnd - skipLoopJump;

        maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

        memoryPointer -= 1;
        deallocateIterator(*forToNode->pidentifier->name);
    }

    void generateForDownToNode(ForDownToNode *forToNode)
    {
        allocateIterator(*forToNode->pidentifier->name);
        long long iterator = getIteratorAddress(*forToNode->pidentifier->name);

        if (auto fromValue = dynamic_cast<IdentifierNode *>(forToNode->fromValue))
        {
            auto it = iteratorMemoryMap.find(*fromValue->name);
            if (fromValue->index)
            {
                generateArrayAccess(fromValue);
                loadArrayValue(memoryPointer);
            }
            else if (it != iteratorMemoryMap.end())
            {
                long long address = getIteratorAddress(*fromValue->name);
                instructions.emplace_back("LOAD", address, true);
            }
            else
            {
                long long address = getVariableMemoryAddress(*fromValue->name);
                instructions.emplace_back("LOAD", address, true);
            }
        }
        else if (auto valueNode = dynamic_cast<ValueNode *>(forToNode->fromValue))
        {
            instructions.emplace_back("SET", valueNode->value, true);
        }

        instructions.emplace_back("STORE", iterator, true);

        if (auto toValue = dynamic_cast<IdentifierNode *>(forToNode->toValue))
        {
            auto it = iteratorMemoryMap.find(*toValue->name);
            if (toValue->index)
            {
                generateArrayAccess(toValue);
                loadArrayValue(memoryPointer);
            }
            else if (it != iteratorMemoryMap.end())
            {
                long long address = getIteratorAddress(*toValue->name);
                instructions.emplace_back("LOAD", address, true);
            }
            else
            {
                long long address = getVariableMemoryAddress(*toValue->name);
                instructions.emplace_back("LOAD", address, true);
            }
        }
        else if (auto valueNode = dynamic_cast<ValueNode *>(forToNode->toValue))
        {
            instructions.emplace_back("SET", valueNode->value, true);
        }

        instructions.emplace_back("STORE", memoryPointer, true);
        long long toValue = memoryPointer++;

        instructions.emplace_back("LOAD", iterator, true);
        long long loopStart = instructions.size() - 1;
        instructions.emplace_back("SUB", toValue, true);
        instructions.emplace_back("JNEG", 0, true);
        long long skipLoopJump = instructions.size() - 1;

        generateCommands(forToNode->commands);

        instructions.emplace_back("SET", -1, true);
        instructions.emplace_back("ADD", iterator, true);
        instructions.emplace_back("STORE", iterator, true);

        instructions.emplace_back("JUMP", loopStart - instructions.size(), true);

        long long loopEnd = instructions.size();
        instructions[skipLoopJump].argument = loopEnd - skipLoopJump;

        maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

        memoryPointer -= 1;
        deallocateIterator(*forToNode->pidentifier->name);
    }

    void generateReadCommand(ReadNode *readNode)
    {
        if (!readNode || !readNode->identifier)
            return;

        auto *identifier = readNode->identifier;
        auto it = iteratorMemoryMap.find(*identifier->name);

        if (procedureCalls.back() != "main")
        {
            if (auto variable = dynamic_cast<IdentifierNode *>(identifier))
            {
                std::cout << "READ: " << procedureCalls.back() << std::endl;
                auto address = getProcedureIdentifierAddress(*variable->name);
                if (address == 1)
                {
                    address = getProcedureArgumentAddress(*variable->name);
                    instructions.emplace_back("GET", 0, true);
                    instructions.emplace_back("STOREI", address, true);
                }
                else if (address == 2)
                {
                    address = getProcedureVariableAddress(*variable->name);
                    instructions.emplace_back("GET", address, true);
                }
                else if (address == 3)
                {
                    maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);
                    generateProcedureArrayAccess(variable);
                    instructions.emplace_back("GET", 0, true);
                    storeArrayValue(memoryPointer);
                }
                else if (address == 4)
                {
                    maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);
                    generateProcedureArgumentsArrayAccess(variable);
                    instructions.emplace_back("GET", 0, true);
                    storeArrayValue(memoryPointer);
                }
            }
        }
        else if (identifier->index)
        {
            generateArrayAccess(identifier);
            instructions.emplace_back("GET", 0, true);
            storeArrayValue(memoryPointer);
        }
        else if (it != iteratorMemoryMap.end())
        {
            long long address = getIteratorAddress(*identifier->name);
            instructions.emplace_back("GET", address, true);
        }
        else
        {
            long long address = getVariableMemoryAddress(*identifier->name);
            instructions.emplace_back("GET", address, true);
        }
        // if (identifier->index)
        // {
        //     generateArrayAccess(identifier);
        //     instructions.emplace_back("GET", 0, true);
        //     storeArrayValue(memoryPointer);
        // }
        // else
        // {
        //     long long address = getVariableMemoryAddress(*identifier->name);
        //     instructions.emplace_back("GET", address, true);
        // }
    }

    void generateCondition(ConditionNode *condition)
    {
        if (auto leftVar = dynamic_cast<IdentifierNode *>(condition->leftValue))
        {
            auto it = iteratorMemoryMap.find(*leftVar->name);
            if (leftVar->index)
            {
                generateArrayAccess(leftVar);
                loadArrayValue(memoryPointer);
            }
            else if (it != iteratorMemoryMap.end())
            {
                long long address = getIteratorAddress(*leftVar->name);
                instructions.emplace_back("LOAD", address, true);
            }
            else
            {
                long long address = getVariableMemoryAddress(*leftVar->name);
                instructions.emplace_back("LOAD", address, true);
            }
        }
        else if (auto valueNode = dynamic_cast<ValueNode *>(condition->leftValue))
        {
            instructions.emplace_back("SET", valueNode->value, true);
        }

        // if (auto leftVar = dynamic_cast<IdentifierNode *>(condition->leftValue))
        // {
        //     if (leftVar->index)
        //     {
        //         generateArrayAccess(leftVar);
        //         loadArrayValue(memoryPointer);
        //     }
        //     else
        //     {
        //         long long address = getVariableMemoryAddress(*leftVar->name);
        //         instructions.emplace_back("LOAD", address, true);
        //     }
        // }
        // else if (auto valueNode = dynamic_cast<ValueNode *>(condition->leftValue))
        // {
        //     instructions.emplace_back("SET", valueNode->value, true);
        // }

        instructions.emplace_back("STORE", memoryPointer, true);
        long long leftTemp = memoryPointer++;

        if (auto rightVar = dynamic_cast<IdentifierNode *>(condition->rightValue))
        {
            auto it = iteratorMemoryMap.find(*rightVar->name);
            if (rightVar->index)
            {
                generateArrayAccess(rightVar);
                loadArrayValue(memoryPointer);
            }
            else if (it != iteratorMemoryMap.end())
            {
                long long address = getIteratorAddress(*rightVar->name);
                instructions.emplace_back("LOAD", address, true);
            }
            else
            {
                long long address = getVariableMemoryAddress(*rightVar->name);
                instructions.emplace_back("LOAD", address, true);
            }
        }
        else if (auto valueNode = dynamic_cast<ValueNode *>(condition->rightValue))
        {
            instructions.emplace_back("SET", valueNode->value, true);
        }

        // if (auto rightVar = dynamic_cast<IdentifierNode *>(condition->rightValue))
        // {
        //     if (rightVar->index)
        //     {
        //         generateArrayAccess(rightVar);
        //         loadArrayValue(memoryPointer);
        //     }
        //     else
        //     {
        //         long long address = getVariableMemoryAddress(*rightVar->name);
        //         instructions.emplace_back("LOAD", address, true);
        //     }
        // }
        // else if (auto valueNode = dynamic_cast<ValueNode *>(condition->rightValue))
        // {
        //     instructions.emplace_back("SET", valueNode->value, true);
        // }

        instructions.emplace_back("SUB", leftTemp, true);

        maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);
        memoryPointer--;
    }

    void generateIfCommand(IfNode *ifNode)
    {
        generateCondition(ifNode->condition);

        if (ifNode->condition->operation == "=")
        {
            instructions.emplace_back("JZERO", 0, true);
            long long condJumpLoc = instructions.size() - 1;

            if (ifNode->elseCommands)
            {
                generateCommands(ifNode->elseCommands);
            }
            long long endElseBlock = instructions.size();
            instructions.emplace_back("JUMP", 0, true);
            long long skipElseJump = instructions.size() - 1;

            generateCommands(ifNode->thenCommands);
            long long endThenBlock = instructions.size();

            instructions[condJumpLoc].argument = endElseBlock - condJumpLoc + 1;
            instructions[skipElseJump].argument = endThenBlock - skipElseJump;
        }
        else if (ifNode->condition->operation == "!=")
        {
            instructions.emplace_back("JZERO", 0, true);
            long long condJumpLoc = instructions.size() - 1;

            generateCommands(ifNode->thenCommands);

            long long endThenBlock = instructions.size();
            long long skipThenJump;
            if (ifNode->elseCommands)
            {
                endThenBlock = instructions.size();
                instructions.emplace_back("JUMP", 0, true);
                skipThenJump = instructions.size() - 1;
                generateCommands(ifNode->elseCommands);
                long long endElseBlock = instructions.size();
                instructions[skipThenJump].argument = endElseBlock - skipThenJump;
                instructions[condJumpLoc].argument = endThenBlock - condJumpLoc + 1;
            }
            else
            {
                instructions[condJumpLoc].argument = endThenBlock - condJumpLoc;
            }
        }
        else if (ifNode->condition->operation == ">")
        {
            instructions.emplace_back("JNEG", 0, true);
            long long condJumpLoc = instructions.size() - 1;

            if (ifNode->elseCommands)
            {
                generateCommands(ifNode->elseCommands);
            }
            long long endElseBlock = instructions.size();
            instructions.emplace_back("JUMP", 0, true);
            long long skipElseJump = instructions.size() - 1;

            generateCommands(ifNode->thenCommands);
            long long endThenBlock = instructions.size();

            instructions[condJumpLoc].argument = endElseBlock - condJumpLoc + 1;
            instructions[skipElseJump].argument = endThenBlock - skipElseJump;
        }
        else if (ifNode->condition->operation == "<")
        {
            instructions.emplace_back("JPOS", 0, true);
            long long condJumpLoc = instructions.size() - 1;

            if (ifNode->elseCommands)
            {
                generateCommands(ifNode->elseCommands);
            }
            long long endElseBlock = instructions.size();
            instructions.emplace_back("JUMP", 0, true);
            long long skipElseJump = instructions.size() - 1;

            generateCommands(ifNode->thenCommands);
            long long endThenBlock = instructions.size();

            instructions[condJumpLoc].argument = endElseBlock - condJumpLoc + 1;
            instructions[skipElseJump].argument = endThenBlock - skipElseJump;
        }
        else if (ifNode->condition->operation == ">=")
        {
            instructions.emplace_back("JPOS", 0, true);
            long long condJumpLoc = instructions.size() - 1;

            generateCommands(ifNode->thenCommands);

            long long endThenBlock = instructions.size();
            long long skipThenJump;
            if (ifNode->elseCommands)
            {
                endThenBlock = instructions.size();
                instructions.emplace_back("JUMP", 0, true);
                skipThenJump = instructions.size() - 1;
                generateCommands(ifNode->elseCommands);
                long long endElseBlock = instructions.size();
                instructions[skipThenJump].argument = endElseBlock - skipThenJump;
                instructions[condJumpLoc].argument = endThenBlock - condJumpLoc + 1;
            }
            else
            {
                instructions[condJumpLoc].argument = endThenBlock - condJumpLoc;
            }
        }
        else if (ifNode->condition->operation == "<=")
        {
            instructions.emplace_back("JNEG", 0, true);
            long long condJumpLoc = instructions.size() - 1;

            generateCommands(ifNode->thenCommands);

            long long endThenBlock = instructions.size();
            long long skipThenJump;
            if (ifNode->elseCommands)
            {
                endThenBlock = instructions.size();
                instructions.emplace_back("JUMP", 0, true);
                skipThenJump = instructions.size() - 1;
                generateCommands(ifNode->elseCommands);
                long long endElseBlock = instructions.size();
                instructions[skipThenJump].argument = endElseBlock - skipThenJump;
                instructions[condJumpLoc].argument = endThenBlock - condJumpLoc + 1;
            }
            else
            {
                instructions[condJumpLoc].argument = endThenBlock - condJumpLoc;
            }
        }
    }

    void generateWriteCommand(WriteNode *writeNode)
    {
        if (!writeNode || !writeNode->value)
            return;

        if (procedureCalls.back() != "main")
        {
            if (auto variable = dynamic_cast<IdentifierNode *>(writeNode->value))
            {
                auto address = getProcedureIdentifierAddress(*variable->name);
                auto it = iteratorMemoryMap.find(*variable->name);
                if (address == 1)
                {
                    address = getProcedureArgumentAddress(*variable->name);
                    instructions.emplace_back("LOADI", address, true);
                    instructions.emplace_back("PUT", 0, true);
                }
                else if (address == 2)
                {
                    address = getProcedureVariableAddress(*variable->name);
                    instructions.emplace_back("PUT", address, true);
                }
                else if (address == 3)
                {
                    maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);
                    generateProcedureArrayAccess(variable);
                    loadArrayValue(memoryPointer);
                    instructions.emplace_back("PUT", 0, true);
                }
                else if (address == 4)
                {
                    maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);
                    generateProcedureArgumentsArrayAccess(variable);
                    loadArrayValue(memoryPointer);
                    instructions.emplace_back("PUT", 0, true);
                }
                else if (it != iteratorMemoryMap.end())
                {
                    long long address = getIteratorAddress(*variable->name);
                    instructions.emplace_back("PUT", address, true);
                }
            }
        }
        else if (auto variable = dynamic_cast<IdentifierNode *>(writeNode->value))
        {
            auto it = iteratorMemoryMap.find(*variable->name);
            if (variable->index)
            {
                generateArrayAccess(variable);
                loadArrayValue(memoryPointer);
                instructions.emplace_back("PUT", 0, true);
            }
            else if (it != iteratorMemoryMap.end())
            {
                long long address = getIteratorAddress(*variable->name);
                instructions.emplace_back("PUT", address, true);
            }
            else
            {
                long long address = getVariableMemoryAddress(*variable->name);
                instructions.emplace_back("PUT", address, true);
            }
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

        if (procedureCalls.back() != "main")
        {
            if (auto variable = dynamic_cast<IdentifierNode *>(assignNode->identifier))
            {
                auto address = getProcedureIdentifierAddress(*variable->name);
                std::cout << "ASSIGN " << *variable->name << std::endl;
                if (address == 1)
                {
                    generateExpression(assignNode->expression);
                    address = getProcedureArgumentAddress(*variable->name);
                    instructions.emplace_back("STOREI", address, true);
                }
                else if (address == 2)
                {
                    generateExpression(assignNode->expression);
                    address = getProcedureVariableAddress(*variable->name);
                    instructions.emplace_back("STORE", address, true);
                }
                else if (address == 3)
                {
                    generateProcedureArrayAccess(variable);
                    long long temp = memoryPointer++;
                    generateExpression(assignNode->expression);
                    storeArrayValue(temp);
                    maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

                    memoryPointer--;
                }
                else if (address == 4)
                {
                    generateProcedureArgumentsArrayAccess(variable);
                    long long temp = memoryPointer++;
                    generateExpression(assignNode->expression);
                    storeArrayValue(temp);
                    maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

                    memoryPointer--;
                }
            }
        }
        else if (auto variable = dynamic_cast<IdentifierNode *>(assignNode->identifier))
        {
            auto it = iteratorMemoryMap.find(*variable->name);
            if (variable->index)
            {
                generateArrayAccess(variable);
                long long temp = memoryPointer++;
                generateExpression(assignNode->expression);
                storeArrayValue(temp);
                maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

                memoryPointer--;
            }
            else if (it != iteratorMemoryMap.end())
            {
                throw std::runtime_error("Cannot assign value to iterator: " + *variable->name);
            }
            else
            {
                generateExpression(assignNode->expression);
                long long address = getVariableMemoryAddress(*variable->name);
                instructions.emplace_back("STORE", address, true);
            }
        }
    }

    void generateExpression(ExpressionNode *expression)
    {
        if (auto binaryExpr = dynamic_cast<BinaryExpressionNode *>(expression))
        {
            bool isLeftArray = false;
            if (procedureCalls.back() != "main")
            {
                if (auto leftVar = dynamic_cast<IdentifierNode *>(binaryExpr->left))
                {
                    auto address = getProcedureIdentifierAddress(*leftVar->name);
                    if (address == 1)
                    {
                        address = getProcedureArgumentAddress(*leftVar->name);
                        instructions.emplace_back("LOADI", address, true);
                        instructions.emplace_back("STORE", memoryPointer, true);
                    }
                    else if (address == 2)
                    {
                        address = getProcedureVariableAddress(*leftVar->name);
                        instructions.emplace_back("LOAD", address, true);
                        instructions.emplace_back("STORE", memoryPointer, true);
                    }
                    else if (address == 3)
                    {
                        generateProcedureArrayAccess(leftVar);
                        isLeftArray = true;
                    }
                    else if (address == 4)
                    {
                        generateProcedureArgumentsArrayAccess(leftVar);
                        isLeftArray = true;
                    }
                }
                else if (auto valueNode = dynamic_cast<ValueNode *>(binaryExpr->left))
                {
                    instructions.emplace_back("SET", valueNode->value, true);
                    instructions.emplace_back("STORE", memoryPointer, true);
                }
            }
            else if (auto leftVar = dynamic_cast<IdentifierNode *>(binaryExpr->left))
            {
                auto it = iteratorMemoryMap.find(*leftVar->name);
                if (leftVar->index)
                {
                    generateArrayAccess(leftVar);
                    isLeftArray = true;
                }
                else if (it != iteratorMemoryMap.end())
                {
                    long long address = getIteratorAddress(*leftVar->name);
                    instructions.emplace_back("LOAD", address, true);
                    instructions.emplace_back("STORE", memoryPointer, true);
                }
                else
                {
                    long long address = getVariableMemoryAddress(*leftVar->name);
                    instructions.emplace_back("LOAD", address, true);
                    instructions.emplace_back("STORE", memoryPointer, true);
                }
            }
            else if (auto valueNode = dynamic_cast<ValueNode *>(binaryExpr->left))
            {
                instructions.emplace_back("SET", valueNode->value, true);
                instructions.emplace_back("STORE", memoryPointer, true);
            }

            long long leftTemp = memoryPointer++;

            bool isRightArray = false;
            if (procedureCalls.back() != "main")
            {
                if (auto rightVar = dynamic_cast<IdentifierNode *>(binaryExpr->right))
                {
                    auto address = getProcedureIdentifierAddress(*rightVar->name);
                    if (address == 1)
                    {
                        address = getProcedureArgumentAddress(*rightVar->name);
                        instructions.emplace_back("LOADI", address, true);
                        instructions.emplace_back("STORE", memoryPointer, true);
                    }
                    else if (address == 2)
                    {
                        address = getProcedureVariableAddress(*rightVar->name);
                        instructions.emplace_back("LOAD", address, true);
                        instructions.emplace_back("STORE", memoryPointer, true);
                    }
                    else if (address == 3)
                    {
                        generateProcedureArrayAccess(rightVar);
                        isRightArray = true;
                    }
                    else if (address == 4)
                    {
                        generateProcedureArgumentsArrayAccess(rightVar);
                        isRightArray = true;
                    }
                }
                else if (auto valueNode = dynamic_cast<ValueNode *>(binaryExpr->right))
                {
                    instructions.emplace_back("SET", valueNode->value, true);
                    instructions.emplace_back("STORE", memoryPointer, true);
                }
            }
            else if (auto rightVar = dynamic_cast<IdentifierNode *>(binaryExpr->right))
            {
                auto it = iteratorMemoryMap.find(*rightVar->name);
                if (rightVar->index)
                {
                    generateArrayAccess(rightVar);
                    isRightArray = true;
                }
                else if (it != iteratorMemoryMap.end())
                {
                    long long address = getIteratorAddress(*rightVar->name);
                    instructions.emplace_back("LOAD", address, true);
                    instructions.emplace_back("STORE", memoryPointer, true);
                }
                else
                {
                    long long address = getVariableMemoryAddress(*rightVar->name);
                    instructions.emplace_back("LOAD", address, true);
                    instructions.emplace_back("STORE", memoryPointer, true);
                }
            }
            else if (auto valueNode = dynamic_cast<ValueNode *>(binaryExpr->right))
            {
                instructions.emplace_back("SET", valueNode->value, true);
                instructions.emplace_back("STORE", memoryPointer, true);
            }

            long long rightTemp = memoryPointer++;

            if (binaryExpr->operation == "+")
            {
                if (isLeftArray && isRightArray)
                {
                    loadArrayValue(leftTemp);
                    instructions.emplace_back("ADDI", rightTemp, true);
                }
                else if (isLeftArray)
                {
                    loadArrayValue(leftTemp);
                    instructions.emplace_back("ADD", rightTemp, true);
                }
                else if (isRightArray)
                {
                    loadArrayValue(rightTemp);
                    instructions.emplace_back("ADD", leftTemp, true);
                }
                else
                {
                    instructions.emplace_back("ADD", leftTemp, true);
                }

                // instructions.emplace_back("LOAD", leftTemp, true);
                // instructions.emplace_back("ADD", rightTemp, true);
                maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

                memoryPointer -= 2;
            }
            else if (binaryExpr->operation == "-")
            {
                if (isLeftArray && isRightArray)
                {
                    loadArrayValue(leftTemp);
                    instructions.emplace_back("SUBI", rightTemp, true);
                }
                else if (isLeftArray)
                {
                    loadArrayValue(leftTemp);
                    instructions.emplace_back("SUB", rightTemp, true);
                }
                else if (isRightArray)
                {
                    instructions.emplace_back("LOAD", leftTemp, true);
                    instructions.emplace_back("SUBI", rightTemp, true);
                }
                else
                {
                    instructions.emplace_back("LOAD", leftTemp, true);
                    instructions.emplace_back("SUB", rightTemp, true);
                }
                maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

                memoryPointer -= 2;
            }
            else if (binaryExpr->operation == "*")
            {
                long long leftValue, rightValue;
                handleArrayValues(isLeftArray, isRightArray, leftTemp, rightTemp, leftValue, rightValue);

                long long resultTemp, signTemp;
                initializeResultAndSign(resultTemp, signTemp);

                handleOperandSign(leftValue, signTemp, true);
                handleOperandSign(rightValue, signTemp, false);

                performMultiplication(leftValue, rightValue, resultTemp);
                applySign(resultTemp, signTemp);

                maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

                memoryPointer -= 4;
                if (isLeftArray)
                    memoryPointer--;
                if (isRightArray)
                    memoryPointer--;
            }
            else if (binaryExpr->operation == "/")
            {
                long long leftValue, rightValue;
                handleArrayValues(isLeftArray, isRightArray, leftTemp, rightTemp, leftValue, rightValue);

                instructions.emplace_back("LOAD", rightValue, true);
                instructions.emplace_back("JZERO", 38, true);

                long long resultTemp, signTemp;
                initializeResultAndSign(resultTemp, signTemp);

                handleOperandSign(leftValue, signTemp, true);
                handleOperandSign(rightValue, signTemp, false);

                performDivision(leftValue, rightValue, resultTemp);
                applySign(resultTemp, signTemp);

                maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

                memoryPointer -= 4;
                if (isLeftArray)
                    memoryPointer--;
                if (isRightArray)
                    memoryPointer--;
            }
            else if (binaryExpr->operation == "%")
            {
                long long leftValue, rightValue;
                handleArrayValues(isLeftArray, isRightArray, leftTemp, rightTemp, leftValue, rightValue);
                // Handle division by zero
                instructions.emplace_back("LOAD", rightValue, true);
                instructions.emplace_back("JZERO", 32, true); // Skip the whole operation if divisor is zero

                instructions.emplace_back("LOAD", leftValue, true);
                instructions.emplace_back("JZERO", 30, true); // Skip the whole operation if divisor is zero

                // Store remainder (initialize with dividend)
                instructions.emplace_back("LOAD", leftValue, true);
                instructions.emplace_back("STORE", memoryPointer, true);
                long long remainderTemp = memoryPointer++;

                // Check if divisor is positive
                instructions.emplace_back("SET", 0, true);
                instructions.emplace_back("SUB", rightValue, true);
                instructions.emplace_back("JPOS", 13, true); // Jump to negative divisor case

                // Positive divisor case
                instructions.emplace_back("LOAD", remainderTemp, true);
                instructions.emplace_back("SUB", rightValue, true);
                instructions.emplace_back("JNEG", 3, true); // While remainder >= divisor
                instructions.emplace_back("STORE", remainderTemp, true);
                instructions.emplace_back("JUMP", -3, true);

                // // Adjust for negative dividend
                instructions.emplace_back("LOAD", remainderTemp, true);
                instructions.emplace_back("JPOS", 5, true);  // Skip if remainder >= 0
                instructions.emplace_back("JZERO", 4, true); // Skip if remainder == 0
                instructions.emplace_back("ADD", rightValue, true);
                instructions.emplace_back("STORE", remainderTemp, true);
                instructions.emplace_back("JUMP", -4, true);
                instructions.emplace_back("JUMP", 12, true); // Skip negative divisor case

                // Negative divisor case
                instructions.emplace_back("LOAD", remainderTemp, true);
                instructions.emplace_back("SUB", rightValue, true);
                instructions.emplace_back("JPOS", 3, true); // While remainder <= divisor
                instructions.emplace_back("STORE", remainderTemp, true);
                instructions.emplace_back("JUMP", -3, true);

                // Adjust for positive dividend
                instructions.emplace_back("LOAD", remainderTemp, true);
                instructions.emplace_back("JNEG", 5, true);  // Skip if remainder <= 0
                instructions.emplace_back("JZERO", 4, true); // Skip if remainder == 0
                instructions.emplace_back("ADD", rightValue, true);
                instructions.emplace_back("STORE", remainderTemp, true);
                instructions.emplace_back("JUMP", -4, true);

                // Load final result
                instructions.emplace_back("LOAD", remainderTemp, true);

                maxMemoryPointer = std::max(maxMemoryPointer, memoryPointer);

                memoryPointer -= 3;
                if (isLeftArray)
                    memoryPointer--;
                if (isRightArray)
                    memoryPointer--;
            }
        }
        else if (procedureCalls.back() != "main")
        {
            if (auto variable = dynamic_cast<IdentifierNode *>(expression))
            {
                auto address = getProcedureIdentifierAddress(*variable->name);
                if (address == 1)
                {
                    address = getProcedureArgumentAddress(*variable->name);
                    instructions.emplace_back("LOADI", address, true);
                }
                else if (address == 2)
                {
                    address = getProcedureVariableAddress(*variable->name);
                    instructions.emplace_back("LOAD", address, true);
                }
                else if (address == 3)
                {
                    generateProcedureArrayAccess(variable);
                    loadArrayValue(memoryPointer);
                }
                else if (address == 4)
                {
                    generateProcedureArgumentsArrayAccess(variable);
                    loadArrayValue(memoryPointer);
                }
            }
            else if (auto valueNode = dynamic_cast<ValueNode *>(expression))
            {
                instructions.emplace_back("SET", valueNode->value, true);
            }
        }
        else if (auto variable = dynamic_cast<IdentifierNode *>(expression))
        {
            auto it = iteratorMemoryMap.find(*variable->name);
            if (variable->index)
            {
                generateArrayAccess(variable);
                loadArrayValue(memoryPointer);
            }
            else if (it != iteratorMemoryMap.end())
            {
                long long address = getIteratorAddress(*variable->name);
                instructions.emplace_back("LOAD", address, true);
            }
            else
            {
                long long address = getVariableMemoryAddress(*variable->name);
                instructions.emplace_back("LOAD", address, true);
            }
        }
        else if (auto valueNode = dynamic_cast<ValueNode *>(expression))
        {
            instructions.emplace_back("SET", valueNode->value, true);
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

    void allocateArray(const std::string &name, long long start, long long end)
    {
        if (arrayMemoryMap.count(name))
        {
            throw std::runtime_error("Array already declared: " + name);
        }
        if (start > end)
        {
            throw std::runtime_error("Invalid array range: start > end");
        }
        long long size = end - start + 1;
        instructions.emplace_back("SET", memoryPointer - start, true);
        memoryPointer += size;
        instructions.emplace_back("STORE", memoryPointer, true);
        arrayMemoryMap[name] = {memoryPointer, size};
        memoryPointer++;
    }

    long long getArrayElementAddress(const std::string &name) const
    {
        auto it = arrayMemoryMap.find(name);
        if (it == arrayMemoryMap.end())
        {
            throw std::runtime_error("Undeclared array: " + name);
        }

        return it->second.first;
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