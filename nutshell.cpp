#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pwd.h>
#include "command.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

//global structures and required functions
int yyparse();
int yylex();
extern FILE* yyin;
extern void yyrestart(FILE*);
std::vector<Command*> cmdTable;
std::vector<char*> strTable;

//
void printPrompt() {
    char cwd[PATH_MAX];
    char host[HOST_NAME_MAX];
    char login[LOGIN_NAME_MAX];
    struct passwd *p = getpwuid(getuid());
    getcwd(cwd, sizeof(cwd));
    gethostname(host, sizeof(host));
    printf(ANSI_COLOR_RED "%s@%s~%s>" ANSI_COLOR_RESET, p->pw_name, host, cwd);
}

void cleanUp() {
    //clear command table
    for (int i = 0; i < cmdTable.size(); i++) {
        delete cmdTable[i];
    }
    cmdTable.clear();

    //clear string values recognized by lex
    for (int i = 0; i < strTable.size(); i++) {
        free(strTable[i]);
    }
    strTable.clear();
}

void testCommandTable() {
    printf("Command table contents:\n");
    for (int i = 0; i < cmdTable.size(); i++) {
        printf("command entry: %s\n", cmdTable[i]->name);
        printf("parameters: ");
        for (int j = 0; j < cmdTable[i]->parameters.size(); j++) {
            printf("%s ", cmdTable[i]->parameters[j]);
        }
        printf("\ninput file: ");
        if (cmdTable[i]->input == nullptr) {
            printf("stdin\n");
        }
        else {
            printf("%s\n", cmdTable[i]->input);
        }
        printf("output file: ");
        if (cmdTable[i]->output == nullptr) {
            printf("stdout\n");
        }
        else {
            printf("%s\n", cmdTable[i]->output);
        }
        printf("\n");
    }
    printf("String table contents:\n");
    for (int i = 0; i < strTable.size(); i++) {
        printf("string entry: %s\n", strTable[i]);
    }
}

int main() {
    system("clear");
    while(1) {
        printPrompt();
        if (yyparse() == 0) {
            printf("success\n");
        };
        testCommandTable();
        yyrestart(yyin);
        cleanUp();
    }
   return 0;
}