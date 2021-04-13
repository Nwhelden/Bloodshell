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
#include <sys/stat.h>
#include <fcntl.h>

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
bool scanStart = true;
bool background = false;
bool endShell = false;

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
            if (cmdTable[i]->outAppend) {
                printf("output is appended\n");
            }
        }
        printf("error file: ");
        if (cmdTable[i]->errToOut) {
            printf("redirected to stdout stream\n");
        }
        else if (cmdTable[i]->err == nullptr) {
            printf("stderr\n");
        }
        else {
            printf("%s\n", cmdTable[i]->err);
        }
        if (background) {
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

void setEnv(char* var, char* word) {
    setenv(var, word, 1);
    for(int i = 0; sizeof(environ), i++) {
        if (environ[i].find(var) != std::string::npos) {
            environ[i] = var + "=" + word;
        }
    } 
}

void unsetEnv(char* var) {
    unsetenv(var);
}

void printEnv() {
    for(int i = 0; environ[i] != NULL; i++) {
        printf("%s\n", environ[i]);
    }
}

void addAlias(char* params[], int size) {
    char* key = params[1];
    char* value = params[0];

    //error cases
    if (size != 2) {
        fprintf(stderr, "Error: invalid usage of 'alias'\n");
        return;
    }
    else if (strcmp(key, value) == 0) {
        fprintf(stderr, "Error: can't set an alias to itself\n");
        return;
    }
    else if (strcmp(key, "unalias") == 0) {
        fprintf(stderr, "Error: can't use 'unalias' as an alias\n");
        return;
    }

    //check for infinite loop (can't have alias as the value of another alias)
    if (aliases.size() != 0) {
        std::map<char*,char*>::iterator iter;
        for (iter = aliases.begin(); iter != aliases.end(); iter++) {

            //expand input key and input value
            //see if they are the same after expansion

            //check if value is an alias, and if that value is assigned to the alias we want to set
            if (strcmp(iter->first, value) == 0) {

                //expand alias for passed value until end, see if the final value matches the passed key (completes circle)
                char* nextKey = iter->first;
                char* nextValue = iter->second;
                std::map<char*,char*>::iterator iter2;
                for (iter2 = aliases.begin(); iter2 != aliases.end(); iter2++) {
                    if (strcmp(iter2->first, nextValue) == 0) {
                        //nextKey = iter2->first;
                        nextValue = iter2->second;
                        iter2 = aliases.begin();
                    }
                }

                if (strcmp(nextValue, key) == 0) {
                    fprintf(stderr, "Error: '%s' is an alias and causes an infinite loop\n", value);
                    return;
                }
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
    char currentDir[strlen(getenv("PWD"))];
    strcpy(currentDir, getenv("PWD"));
    if (file[0] != '/') {
        strcat(currentDir, "/");
        strcat(currentDir, file);
        if (chdir(currentDir) == 0) {
            setenv("PWD", currentDir, 1);
        } else {
            std::cout << "Directory not found" << std::endl;
        }
    } else {
        strcat(currentDir, "/");
        strcat(currentDir, file);
        if (chdir(currentDir) == 0) {
            setenv("PWD", currentDir, 1);
        } else {
            std::cout << "Directory not found" << std::endl;
        }
    }
}

bool checkFiles(int index) {
    char* currDir = getenv("PWD");

    //check input (if you can read from file)
    if (cmdTable[i]->input != nullptr) {
        char* fileName = cmdTable[index]->input;
        char filePath[strlen(currDir) + 1 + strlen(fileName)];
        strcpy(filePath, currDir);
        strcat(filePath, "/");
        strcat(filePath, fileName);

        if (access(filePath, F_OK) == 0) {
            if(access(filePath, R_OK) != 0) {
                printf("Error: permission to read '%s' denied", fileName)
                return false;
            }
        }
        else {
            printf("Error: '%s' not found", fileName)
            return false;
        }
    }

    //check output (if you can write to file)
    if (cmdTable[i]->output != nullptr) {
        char* fileName = cmdTable[index]->input;
        char filePath[strlen(currDir) + 1 + strlen(fileName)];
        strcpy(filePath, currDir);
        strcat(filePath, "/");
        strcat(filePath, fileName);

        if (access(filePath, F_OK) == 0) {
            if(access(filePath, W_OK) != 0) {
                printf("Error: permission to write to '%s' denied", fileName)
                return false;
            }
        }
        else {
            printf("Error: '%s' not found", fileName)
            return false;
        }        
    }

    //need to also check if err is being output to valid file
    if (cmdTable[i]->err != nullptr) {
        char* fileName = cmdTable[index]->input;
        char filePath[strlen(currDir) + 1 + strlen(fileName)];
        strcpy(filePath, currDir);
        strcat(filePath, "/");
        strcat(filePath, fileName);

        if (access(filePath, F_OK) == 0) {
            if(access(filePath, W_OK) != 0) {
                printf("Error: permission to write to '%s' denied", fileName)
                return false;
            }
        }
        else {
            printf("Error: '%s' not found", fileName)
            return false;
        }  
    }
    return true;
}

bool checkCommand(char* name, int index) {
    //check if built-in
    if (strcmp(name, "setenv") == 0) {
        return true;
    } else if (strcmp(name, "unsetenv") == 0) {
        return true;
    } else if (strcmp(name, "printenv") == 0) {
        return true;
    } else if (strcmp(name, "alias") == 0) {
        return true;
    } else if (strcmp(name, "unalias") == 0) {
        return true;
    } else if (strcmp(name, "cd") == 0) {
        return true;
    } else if (strcmp(name, "bye") == 0) {
        return true;
    }

    //otherwise check if general
    char* path = getenv("PATH");
    char temp[strlen(path)];
    strcpy(temp, path);
    char* delim = strtok(path, ":");
    setenv("PATH", temp, 1);

    while (delim != NULL) {
        char* filePath = new char[strlen(name) + strlen(delim) + 1]; //mem-leak
        strcpy(filePath, delim);
        strcat(filePath, "/");
        strcat(filePath, name);
        if (access(filePath, F_OK) == 0) {
            if (access(filePath, X_OK) == 0) {
                cmdTable[index]->filepath = filePath;
                return true;
            }
            else {
                printf("Error: permission to execute '%s' denied\n", name);
            }
        }
        delim = strtok(NULL, ":");
    }
    printf("Error: command '%s' does not exist in PATH\n", name);
    return false;
}

void inputRedirect(int index) {
    //can only have input redirection for general commands
    if (cmdTable[index]->filepath != nullptr)
        if (cmdTable[index]->input != nullptr) {
            //note: file existence and accessability already checked
            int fd = open(cmdTable[index]->input, O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd); 
        }
    }
    else { 
        printf("Error: '%s' can't be directed input", cmdTable[index]->name)
        exit(1); //inputRedirect is a helper function for child process, so end process on error
    }
}

void outputRedirect(int index) {
    //can only have output redirection for general commands and select built in
    char* name cmdTable[index]->name;
    if (cmdTable[index]->filepath != nullptr || strcmp(name, "alias") == 0 || strcmp(name, "printenv") == 0) {
        //stdout redirection
        if (cmdTable[index]->output != nullptr) {
            if (cmdTable[index]->outAppend) {
                int fd = open(cmdTable[index]->output, O_WRONLY | O_CREAT | O_APPEND);
            }
            else {
                int fd = open(cmdTable[index]->output, O_WRONLY | O_CREAT);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        //stderr redirection
        if (cmdTable[index]->err != nullptr) {
            if (cmdTable[index]->errToOut) {
                dup2(STDERR_FILENO, STDOUT_FILENO);
            }
            else {
                int fd = open(cmdTable[index]->err, O_WRONLY | O_CREAT);
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }
    }
    else {
        printf("Error: '%s' can't redirect output", cmdTable[index]->name)
        exit(1);
    }
}

void setupOnly(char* params[], int size, int index) {
    //setup IO as needed
    inputRedirect(index);
    outputRedirect(index);

    char* name = cmdTable[index]->name;
    if (strcmp(name, "setenv") == 0) {
        setEnv(params, size);
    } else if (strcmp(name, "unsetenv") == 0) {
        unsetEnv(params, size);
    } else if (strcmp(name, "printenv") == 0) {
        printEnv(params, size); 
    } else if (strcmp(name, "alias") == 0) {
        if (size == 0) {
            printAliases();
        } else {
            addAlias(params, size);
        }
    } else if (strcmp(name, "unalias") == 0) {
        removeAlias(params, size);
    } else if (strcmp(name, "cd") == 0) {
        changeDir(params, size);
    } else if (strcmp(name, "bye") == 0) {
        endShell = true;
    } 
}

void setupFirst(char* params[], int index, int pipes[]) {
    //setup IO as needed
    inputRedirect(index);

    char* name cmdTable[index]->name;
    if (params[0] != nullptr || strcmp(name, "alias") == 0 || strcmp(name, "alias") == 0) {
        //child replaces stdout with the write-end of the pipe and closes pipes
        dup2(pipes[index][1], STDOUT_FILENO);
        close(pipes[index][0]);
        close(pipes[index][1]);
        //execv(filePath, params);
    }
    else {
        printf("Error: output of '%s' can't be piped", cmdTable[index]->name)
        exit(1);
    }
}

void setupMiddle() {

}

void setupLast() {

}

int processCommand() {
    //check if commands exist and are executable
    for (int i = 0; i < cmdTable.size(); i++) {
        if (!checkCommand(cmdTable[i]->name, i)) {
            return 1;
        }
    }

    //check if files exist and are accessible
    for (int i = 0; i < cmdTable.size(); i++) {
        if (!checkFiles(i)) {
            return 1;
        }
    }

    //create pipes as needed
    std::vector<int*> pipes;
    for (int i = 0; i < cmdTable.size() - 1; i++) {
        int* pipe = new int[2];
        pipes.push_back(pipe);
    }

    //keeps track of pid
    std::vector<pid_t> children;

    //parent forks for each command in command line
    pid_t pid;
    for (int i = 0; i < cmdTable.size(); i++) {

        //reorganize parameters into array
        char* params[cmdTable[i]->parameters.size() + 2];
        params[0] = cmdTable[i]->filepath;
        params[cmdTable[i]->parameters.size() + 1] = NULL;
        int k = 1;
        for (int j = cmdTable[i]->parameters.size() - 1; j >= 0; --j) {
            params[k] = cmdTable[i]->parameters[j];
            k++;
        }

        char* name = cmdTable[i]->name;
        int size = cmdTable[i]->parameters.size();

        //execute command
        pid = fork();
        if (pid < 0) {
            printf("Error: fork failed for command '%s'\n", name);
            return 1;
        }
        else if (pid == 0) {
            if (cmdTable.size() == 1) { //no pipes
                //setup function sets up IO redirection and handles built-in commands
                setupOnly(params, i, size);
                //exit(0); //if built-in is executed
            }
            else if (i == 0) { //first command in pipe
                setupFirst(params, i, pipes[i]);
                if (params[0] != nullptr) {
                    execv(params[0], params);
                    perror("execv");
                    exit(1);
                }
            }
            else if () { //middle of pipe
                if (params[0] != nullptr) {
                    execv(params[0], params);
                    perror("execv");
                    exit(1);
                }
            }
            else { //end of pipe
                if(!executeLast(name, params, i)) {
                    perror("execv");
                    exit(1);
                }
                exit(0); //if built-in is executed
            }

            //close child's copies of pipes
            for () {

            }

            //if filepath == nullptr, then a built-in executed 
            if (params[0] != nullptr) {
                execv(params[0], params); //exec only returns if there is an error
                perror("execv");
                exit(1);
            }
            exit(0); //exec already returns on success for general commands, need to return for built-in
        }
        children.push_back(pid);
    }

    //close parent's copies of pipes
    

    //after creating children, wait if needed
    if (background == false) {
        for (int i = 0; i < cmdTable.size(); i++) {
            int status;
            wait(&status);
            if (WEXITSTATUS(status) == ) 
        }
    }
    return 0;
}

void shell_init() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    setenv("PATH", ".:/bin", 1);
    setenv("HOME", cwd, 1);
    setenv("PWD", cwd, 1);
}

void printPrompt() {
    char cwd[PATH_MAX];
    char host[HOST_NAME_MAX];
    char login[LOGIN_NAME_MAX];
    struct passwd *p = getpwuid(getuid());
    getcwd(cwd, sizeof(cwd));
    gethostname(host, sizeof(host));
    printf(ANSI_COLOR_RED "%s@%s:%s>" ANSI_COLOR_RESET, p->pw_name, host, cwd);
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
    background = false;
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

int main() {
    system("clear");
    shell_init();
    while(1) {
        printPrompt();
        if (yyparse() == 0) {
            if (processCommand() == 0) {
                if (endShell) {
                    printf("Successfully exited shell\n");
                    break;
                }
            }
        }
        //parser error already handled in .y

        //testCommandTable();
        yyrestart(yyin);
        cleanUp();
    }
    freeMem();

   return 0;
}