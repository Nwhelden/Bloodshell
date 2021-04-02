#include <sys/types.h>      
#include <sys/wait.h>         
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <map>
#include <iostream>
#include "json_util.h"

int yyparse();
extern char ** environ;
char cwd[PATH_MAX];

int executeGeneral(std::string name, std::vector<std::string> params) {
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        printf("Error. Try again.\n");
        return EXIT_FAILURE;
    } else {
        if (pid == 0) {
            execv(name, params);
            printf("Success");
            return EXIT_SUCCESS;
        } else {
            wait(NULL);
            EXIT_SUCCESS;
        }
    }
}

void shell_init() {
    getcwd(cwd, sizeof(cwd));
    for(int i = 0; sizeof(environ), i++) {
        if (environ[i].substr(0,4) == "PATH") {
            environ[i] = "PATH=.:/bin";
        }
        if (environ[i].substr(0,4) == "HOME") {
            environ[i] = "HOME=" + cwd;
        }
    }
}

void setEnv(std:string var, std:string word) {
    for(int i = 0; sizeof(environ), i++) {
        if (environ[i].find(var) != std::string::npos) {
            environ[i] = var + "=" + word;
        }
    }
}

void unsetEnv(std:string var) {
    unsetenv(var);
}

void printEnv() {
    for(int i = 0; sizeof(environ), i++) {
        printf("%s\n", environ[i]);
    }
}

void checkGeneral(std:string name, std::vector<std::string> params) {
    std::string pPath = getenv("PATH");
    pPath += "/"
    pPath += name;
    if (access(pPath, F_OK) = 0) {
        vector<std::string> newParams;
        newParams.push_back(name)
        for (int i = 0; i < params.size(); i++) {
            newParams.push_back(params[i]);
        }
        newParams.push_back(NULL);
        executeGeneral(newParams);
    };
}

void addAlias(std::string key, std::vector<std::string> params) {
    std:string command = "";
    for (int i = 1; i < params.size(); i++) {
        command = command + " " + params[i];
    }
    aliases[key] = command;
}

void removeAlias(std::string key) {
    aliases.erase(key);
}

void executeAlias(std::string command) {
    
}

void printAliases() {
    for (iter = aliases.begin(); iter != aliases.end(); iter++) {
        std::cout << iter->first << " = " << iter->second << std::endl;
    }
}

void changeDir(std::string file) {
    std::string currentDir = getenv("PWD")
    if (file[0] != '/' ) {
        setEnv("PWD", currentDir + '/' + file);
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
    for (int i = 0; i < Command.size(); i++) {
        if (aliases.find(Command[i].name) != aliases.end()) {
            executeAlias(aliases[Command[i].name]);
        } else {
            switch(Command[i].name) {
                case "setenv": setEnv(Command[i].parameters[0], Command[i].parameters[1]); return 0;
                case "unsetenv": unsetEnv(Command[i].parameters[0]); return 0;
                case "printenv": printEnv(); return 0;
                case "bye": return 1;
                case "alias": if (Command[i].parameters.size() == 0) {
                                    printAliases();
                            } else {
                                    addAlias(Command[i].parameters[1], Command[i].parameters); return 0;
                            }
                case "unalias": removeAlias(Command[i].parameters[1]); return 0;
                case "cd": changeDir(Command[i].parameters[1]);
                default: executeGeneral(Command[i].name, Command[i].parameters); return 0;
        }
    }
}