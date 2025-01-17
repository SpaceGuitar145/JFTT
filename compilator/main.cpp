#include <iostream>
#include <cstdio>
#include "AstNode.hpp"
#include "CodeGenerator.hpp"

extern FILE* yyin;
extern int yyparse();
extern int yydebug;
extern ProgramNode* root;

int main(int argc, char** argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            std::cerr << "Failed to open file: " << argv[1] << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    std::cout << "Parsing input..." << std::endl;
    if (yyparse() == 0) {
        std::cout << "Parsing completed successfully." << std::endl;

        if (root) {
            std::cout << "Generated AST:" << std::endl;
            root->print();

            try {
                CodeGenerator generator;
                generator.generateProgram(root);
                std::cout << "Generated Instructions:" << std::endl;
                generator.printInstructions();
            } catch (const std::runtime_error& e) {
                std::cerr << "Error during code generation: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "No AST generated." << std::endl;
        }
    } else {
        std::cerr << "Parsing failed." << std::endl;
    }

    if (yyin) fclose(yyin);

    delete root;

    return 0;
}
