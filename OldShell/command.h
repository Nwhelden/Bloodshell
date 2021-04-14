#pragma once
#include <vector>

struct Command {
    char* name;
    std::vector<char*> parameters;
    char* input;
    char* output;
    Command(char* name);
};