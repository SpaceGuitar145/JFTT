#include <iostream>
#include <fstream>
#include <cstdio>
#include "AstNode.hpp"
#include "CodeGenerator.hpp"

extern FILE* yyin;
extern int yyparse();
extern int yydebug;
extern ProgramNode* root;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        std::cerr << "Failed to open input file: " << argv[1] << std::endl;
        return 1;
    }

    std::cout << "Parsing input..." << std::endl;
    if (yyparse() == 0) {
        std::cout << "Parsing completed successfully." << std::endl;

        if (root) {
            try {
                CodeGenerator generator;
                generator.generateProgram(root);
                
                std::ofstream outFile(argv[2]);
                if (!outFile.is_open()) {
                    std::cerr << "Failed to open output file: " << argv[2] << std::endl;
                    fclose(yyin);
                    return 1;
                }

                auto cout_buf = std::cout.rdbuf();
                std::cout.rdbuf(outFile.rdbuf());

                generator.printInstructions();

                std::cout.rdbuf(cout_buf);
                outFile.close();

                std::cout << "Code generation completed successfully." << std::endl;

            } catch (const std::runtime_error& e) {
                std::cerr << e.what() << std::endl;
                fclose(yyin);
                return 1;
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
