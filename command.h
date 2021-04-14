#pragma once
#include <vector>

struct Command {
    char* name;
    std::vector<char*> parameters;
    bool builtIn;
    bool outAppend;
    bool errToOut;
    char* input;
    char* output;
    char* err;
    char* filepath;
    Command(char* name);
};