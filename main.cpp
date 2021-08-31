#include <iostream>
#include <fstream>
#include <streambuf>

#include "compiler/RocCompiler.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Expected input file";
        return 1;
    }
    char* toCompile = argv[1];
    std::string asStr = std::string(toCompile);

    if(asStr.substr(asStr.find_last_of(".") + 1) != "roc") {
        std::cerr << "Expected input Roc lang file";
        return 1;
    }

    std::string line;
    std::ifstream f(asStr);

    if (!f.good()) {
        std::cerr << "Input file does not exist";
        return 1;
    }

    std::string acc;

    if (f.is_open())
    {
        while (getline(f, line))
        {
            if (!acc.empty()) acc+='\n';
            acc+=line;
        }
        f.close();
    }

    RocCompiler::compile(acc, "output.s");

    return 0;
}
