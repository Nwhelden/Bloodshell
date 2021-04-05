#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <pwd.h>
#include "command.h"
#include <cstring>
#include <map>
#include <iostream>
#include <vector>
#include <sys/wait.h>
#include <sys/types.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

//global structures and required functions
int yyparse();
int yylex();
extern FILE* yyin;
extern void yyrestart(FILE*);
std::vector<Command*> cmdTable;
std::vector<char*> strTable;
extern char ** environ;
std::map<char*, char*> aliases;
bool scanStart;

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

    scanStart = true;    
}

void freeMem() {
    //clear alias table
    std::map<char*,char*>::iterator iter;
    for (iter = aliases.begin(); iter != aliases.end(); iter++) {
        free(iter->second);
        free(iter->first);
    }
    aliases.clear();
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
        if (cmdTable[i]->background) {
            printf("command executed in background\n");
        }
        else {
            printf("shell waits for command to finish\n");
        }
        printf("\n");
    }
    printf("String table contents:\n");
    for (int i = 0; i < strTable.size(); i++) {
        printf("string entry: %s\n", strTable[i]);
    }
}

int executeGeneral(char* path, char* params[]) {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        printf("Error. Try again.\n");
        return EXIT_FAILURE;
    } else {
        if (pid == 0) {
            execv(path, params);
            printf("Success\n");
            return EXIT_SUCCESS;
        } else {
            wait(NULL);
            EXIT_SUCCESS;
        }
    }
}

void shell_init() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    setenv("PATH", ".:/bin", 1);
    setenv("HOME", cwd, 1);
    setenv("PWD", cwd, 1);
    /*
    for(int i = 0; sizeof(environ), i++) {
        if (environ[i].substr(0,4) == "PATH") {
            environ[i] = "PATH=.:/bin";
        }
        if (environ[i].substr(0,4) == "HOME") {
            environ[i] = "HOME=" + cwd;
        }
    }
    */
}

void setEnv(char* var, char* word) {
    setenv(var, word, 1);
    /*
    for(int i = 0; sizeof(environ), i++) {
        if (environ[i].find(var) != std::string::npos) {
            environ[i] = var + "=" + word;
        }
    }
    */
}

void unsetEnv(char* var) {
    unsetenv(var);
}

void printEnv() {
    for(int i = 0; i < sizeof(environ); i++) {
        printf("%s\n", environ[i]);
    }
}

void checkGeneral(char* name, char* params[], int size) {
    char* path = new char[strlen(name) + 5];
    strcpy(path, "/bin");
    strcat(path, "/");
    strcat(path, name);
    if (access(path, F_OK) == 0) {
        char* newParams[size + 2];
        newParams[0] = path;
        for (int i = 0; i < size; i++) {
            newParams[i + 1] = params[i];
        }
        newParams[size + 1] = NULL;
        executeGeneral(path, newParams);
    }
}

void addAlias(char* params[], int size) {
    if (size != 2) {
        fprintf(stderr, "Error: invalid usage of 'alias'\n");
        return;
    }

    char* key = params[1];
    char* value = params[0];

    //check for infinite loop (can't have alias as the value of another alias)
    if (aliases.size() != 0) {
        std::map<char*,char*>::iterator iter;
        for (iter = aliases.begin(); iter != aliases.end(); iter++) {
            if (strcmp(iter->first, value) == 0) {
                fprintf(stderr, "Error: '%s' is an alias\n", value);
                return;
            }
            else if (strcmp(iter->first, key) == 0) {
                fprintf(stderr, "Error: alias for '%s' already exists\n", key);
                return;
            }
        }
    }

    //parameters will be forgotten if not dynamically allocated
    char* temp1 = strdup(key);
    char* temp2 = strdup(value);
    aliases[temp1] = temp2;
}

void removeAlias(char* key) {
    //key and value of alias map point to text on heap
    std::map<char*,char*>::iterator iter;
    for (iter = aliases.begin(); iter != aliases.end(); iter++) {
        if (strcmp(iter->first, key) == 0) {
            free(iter->second);
            free(iter->first);
        }
    }

    aliases.erase(key);
}

void printAliases() {
    std::map<char*,char*>::iterator iter;
    for (iter = aliases.begin(); iter != aliases.end(); iter++) {
        std::cout << iter->first << " = " << iter->second << std::endl;
    }
}

void changeDir(char* file) {
    char* currentDir = getenv("PWD");
    if (file[0] != '/') {
        strcat(currentDir, "/");
        strcat(currentDir, file);
        setenv("PWD", currentDir, 1);
        if (chdir(getenv("PWD")) != 0) {
            std::cout << "Directory not found" << std::endl;
        }
    } else {
        if (chdir(file) != 0) {
            std::cout << "Directory not found" << std::endl;
        }
    }
}

int processCommand() {
    for (int i = 0; i < cmdTable.size(); i++) {
        char* name = cmdTable[i]->name;
        char* params[cmdTable[i]->parameters.size()];
        int size = sizeof(params)/sizeof(char*);
        for (int j = 0; j < cmdTable[i]->parameters.size(); j++) {
            params[j] = cmdTable[i]->parameters[j];
        }

        //go through built-in 
        if (strcmp(name, "setenv") == 0) {
            setEnv(params[0], params[1]);
            return 0;
        } else if (strcmp(name, "unsetenv") == 0) {
            unsetEnv(params[0]);
            return 0;
        } else if (strcmp(name, "printenv") == 0) {
            printEnv(); return 0;
        } else if (strcmp(name, "alias") == 0) {
            if (cmdTable[i]->parameters.size() == 0) {
                printAliases();
                return 0;
            } else {
                addAlias(params, size);
                return 0;
            }
        } else if (strcmp(name, "unalias") == 0) {
            removeAlias(params[1]);
            return 0;
        } else if (strcmp(name, "cd") == 0) {
            changeDir(params[0]);
        } else if (strcmp(name, "bye") == 0) {
            return 1;
        } else {
            checkGeneral(name, params, size);
            return 0;
        }
    }
}

int main() {
    system("clear");
    shell_init();
    while(1) {
        printPrompt();
        if (yyparse() == 0) {
            if (processCommand() == 1) {
                printf("Successfully exited shell\n");
                break;
            }
        }
        //parser error already handled in .y

        testCommandTable();
        yyrestart(yyin);
        cleanUp();
    }
    freeMem();

   return 0;
}