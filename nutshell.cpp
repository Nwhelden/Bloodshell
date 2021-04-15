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
char cwd[PATH_MAX];
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

int setEnv(char* params[], int size) {
    //error cases
    if (size != 2) {
        fprintf(stderr, "Error: invalid parameters for 'setenv'\n");
        return 1;
    }

    setenv(params[1], params[2], 1);
    return 0;
}

int unsetEnv(char* params[], int size) {
    //error cases
    if (size != 1) {
        fprintf(stderr, "Error: invalid parameters for 'unsetenv'\n");
        return 1;
    }
    else if (strcmp(params[1], "PATH") == 0) {
        fprintf(stderr, "Error: cannot unset 'PATH' variable\n");
        return 1;
    }
    else if (strcmp(params[1], "HOME") == 0) {
        fprintf(stderr, "Error: cannot unset 'HOME' variable\n");
        return 1;
    }

    unsetenv(params[1]);
    return 0;
}

int printEnv(int size) {
    //error cases
    if (size != 0) {
        fprintf(stderr, "Error: invalid parameters for 'printenv'\n");
        return 1;
    }

    for(int i = 0; environ[i] != NULL; i++) {
        printf("%s\n", environ[i]);
    }
    return 0;
}

int addAlias(char* params[], int size) {
    //error cases
    if (size != 2) {
        fprintf(stderr, "Error: invalid parameters for 'alias'\n");
        return 1;
    }

    char* key = params[1];
    char* value = params[2];
    if (strcmp(key, value) == 0) {
        fprintf(stderr, "Error: can't set an alias to itself\n");
        return 1;
    }
    else if (strcmp(key, "unalias") == 0) {
        fprintf(stderr, "Error: can't use 'unalias' as an alias\n");
        return 1;
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
                    return 1;
                }
            }
            else if (strcmp(iter->first, key) == 0) {
                fprintf(stderr, "Error: alias for '%s' already exists\n", key);
                return 1;
            }
        }
    }

    //parameters will be forgotten if not dynamically allocated
    char* temp1 = strdup(key);
    char* temp2 = strdup(value);
    aliases[temp1] = temp2;
    return 0;
}

int removeAlias(char* params[], int size) {
    //error cases
    if (size != 1) {
        fprintf(stderr, "Error: invalid parameters for 'unalias'\n");
        return 1;
    }

    char* key = params[1];

    //key and value of alias map point to text on heap
    std::map<char*,char*>::iterator iter;
    for (iter = aliases.begin(); iter != aliases.end(); iter++) {
        if (strcmp(iter->first, key) == 0) {
            free(iter->second);
            free(iter->first);
            aliases.erase(iter->first);
            return 0;
        }
    }

    fprintf(stderr, "Error: alias for '%s' not found\n", key);
    return 1;
}

void printAliases() {
    std::map<char*,char*>::iterator iter;
    for (iter = aliases.begin(); iter != aliases.end(); iter++) {
        std::cout << iter->first << " = " << iter->second << std::endl;
    }
}

int changeDir(char* params[], int size) {
    if (size == 0) {
        chdir(getenv("HOME"));
        setenv("PWD", getenv("HOME"), 1);
        return 0;
    }
    else if (size > 1) {
        fprintf(stderr, "Error: invalid parameters for 'cd'\n");
        return 1; 
    }

    char* file = params[1];
    char currentDir[strlen(getenv("PWD"))];
    strcpy(currentDir, getenv("PWD"));
    if (file[0] != '/') {
        strcat(currentDir, "/");
        strcat(currentDir, file);
        if (chdir(currentDir) == 0) {
            getcwd(cwd, sizeof(cwd));
            setenv("PWD", cwd, 1);
        } else {
            fprintf(stderr, "Error: directory '%s' not found\n", file);
            return 1;
        }
    } else {
        if (chdir(file) == 0) {
            getcwd(cwd, sizeof(cwd));
            setenv("PWD", cwd, 1);
        } else {
            fprintf(stderr, "Error: directory '%s' not found\n", file);
            return 1;
        }
    }

    return 0;
}

bool checkFiles(int index) {
    char* currDir = getenv("PWD");

    //check input (if you can read from file)
    if (cmdTable[index]->input != nullptr) {
        char* fileName = cmdTable[index]->input;
        char filePath[strlen(currDir) + 1 + strlen(fileName)];
        strcpy(filePath, currDir);
        strcat(filePath, "/");
        strcat(filePath, fileName);

        if (access(filePath, F_OK) == 0) {
            if(access(filePath, R_OK) != 0) {
                fprintf(stderr, "Error: permission to read '%s' denied\n", fileName);
                return false;
            }
        }
        else {
            fprintf(stderr, "Error: file '%s' not found\n", fileName);
            return false;
        }
    }

    //check output (if you can write to file)
    if (cmdTable[index]->output != nullptr) {
        char* fileName = cmdTable[index]->output;
        char filePath[strlen(currDir) + 1 + strlen(fileName)];
        strcpy(filePath, currDir);
        strcat(filePath, "/");
        strcat(filePath, fileName);

        if (access(filePath, F_OK) == 0) {
            if(access(filePath, W_OK) != 0) {
                fprintf(stderr, "Error: permission to write to '%s' denied\n", fileName);
                return false;
            }
        }
        else {
            fprintf(stderr, "Error: file '%s' not found\n", fileName);
            return false;
        }        
    }

    //need to also check if err is being output to valid file
    if (cmdTable[index]->err != nullptr) {
        char* fileName = cmdTable[index]->err;
        char filePath[strlen(currDir) + 1 + strlen(fileName)];
        strcpy(filePath, currDir);
        strcat(filePath, "/");
        strcat(filePath, fileName);

        if (access(filePath, F_OK) == 0) {
            if(access(filePath, W_OK) != 0) {
                fprintf(stderr, "Error: permission to write to '%s' denied\n", fileName);
                return false;
            }
        }
        else {
            fprintf(stderr, "Error: file '%s' not found\n", fileName);
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
                fprintf(stderr, "Error: permission to execute '%s' denied\n", name);
                return false;
            }
        }
        delim = strtok(NULL, ":");
    }
    fprintf(stderr, "Error: command '%s' does not exist in PATH\n", name);
    return false;
}

void inputRedirect(int index) {
    if (cmdTable[index]->input != nullptr) {
        //can only have input redirection for general commands
        if (cmdTable[index]->filepath != nullptr) {
            //note: file existence and accessability already checked
            int fd = open(cmdTable[index]->input, O_RDONLY);
            dup2(fd, STDIN_FILENO);
            close(fd); 
        }
        else { 
            fprintf(stderr, "Error: '%s' can't be directed input", cmdTable[index]->name);
            exit(1); //inputRedirect is a helper function for child process, so end process on error
        }   
    }
}

void outputRedirect(int index) {
    //stdout redirection
    if (cmdTable[index]->output != nullptr) {
        //can only have output redirection for general commands and select built in
        char* name = cmdTable[index]->name;
        if (cmdTable[index]->filepath != nullptr || strcmp(name, "alias") == 0 || strcmp(name, "printenv") == 0) {
            int fd;
            if (cmdTable[index]->outAppend) {
                fd = open(cmdTable[index]->output, O_WRONLY | O_APPEND);
            }
            else {
                fd = open(cmdTable[index]->output, O_WRONLY | O_TRUNC);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        else {
            fprintf(stderr, "Error: '%s' can't redirect output", cmdTable[index]->name);
            exit(1);
        }
    }

    //stderr redirection
    if (cmdTable[index]->err != nullptr || cmdTable[index]->errToOut) { 
        char* name = cmdTable[index]->name;
        if (cmdTable[index]->filepath != nullptr || strcmp(name, "alias") == 0 || strcmp(name, "printenv") == 0) { 
            //stderr redirection
            if (cmdTable[index]->errToOut) {
                dup2(STDOUT_FILENO, STDERR_FILENO);
            }
            else {
                int fd = open(cmdTable[index]->err, O_WRONLY );
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }
        else {
            fprintf(stderr, "Error: '%s' can't redirect error", cmdTable[index]->name);
            exit(1);
        }   
    }
}

int builtIn(char* params[], int size, int index) {
    //if there is error redirection, set stderr to file
    int savedFd = -1;
    if (cmdTable[index]->err != nullptr) {
        savedFd = dup(1);
        if (cmdTable[index]->errToOut) {
            dup2(STDOUT_FILENO, STDERR_FILENO);
        }
        else {
            int fd = open(cmdTable[index]->err, O_WRONLY);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
    }

    int rVal;
    char* name = cmdTable[index]->name;
    if (strcmp(name, "setenv") == 0) {
        rVal = setEnv(params, size);
    } else if (strcmp(name, "unsetenv") == 0) {
        rVal = unsetEnv(params, size);
    } else if (strcmp(name, "printenv") == 0) {
        rVal = printEnv(size);
    } else if (strcmp(name, "alias") == 0) {
        if (size != 0) {
            rVal = addAlias(params, size);
        } else {
            printAliases();
            rVal = 0;
        }
    } else if (strcmp(name, "unalias") == 0) {
        rVal = removeAlias(params, size);
    } else if (strcmp(name, "cd") == 0) {
        rVal = changeDir(params, size);
    } else if (strcmp(name, "bye") == 0) {
        endShell = true;
        rVal = 0;
    } 

    //restore stderr to terminal
    if (cmdTable[index]->err != nullptr) {
        dup2(savedFd, STDERR_FILENO);
        close(savedFd);
    }

    return rVal;
} 

void setupOnly(char* params[], int size, int index) {
    //setup IO as needed
    inputRedirect(index);
    outputRedirect(index);

    //run built-ins that have IO redirection
    char* name = cmdTable[index]->name;
    if (strcmp(name, "printenv") == 0) {
        if (printEnv(size) == 1) {
            exit(1);
        }
    } else if (strcmp(name, "alias") == 0) {
        printAliases();
    }
}

void setupFirst(char* params[], int size, int index, int* pipe) {
    //setup IO as needed
    inputRedirect(index);

    char* name = cmdTable[index]->name;
    if (params[0] != nullptr || (strcmp(name, "alias") == 0 && size == 0) || strcmp(name, "printenv") == 0) {
        //child replaces stdout with the write-end of the pipe
        dup2(pipe[1], STDOUT_FILENO);
        if (strcmp(name, "printenv") == 0) {
            if (printEnv(size) == 1) {
                exit(1);
            }
        }
        else if (strcmp(name, "alias") == 0) {
            printAliases();
        }
    }
    else {
        fprintf(stderr, "Error: '%s' can't be piped\n", cmdTable[index]->name);
        exit(1);
    }
}

void setupMiddle(char* params[], int index, int* pipe1, int* pipe2) {
    //can't have IO as middle process

    char* name = cmdTable[index]->name;
    if (params[0] != nullptr) {
        //read from the previous pipe
        //redirect stdout into the write-end of the next pipe
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);
    }
    else {
        printf("Error: output of '%s' can't be piped", cmdTable[index]->name);
        exit(1);
    }
}

void setupLast(char* params[], int index, int* pipe) {
    //can have input redirected as final process
    outputRedirect(index);

    char* name = cmdTable[index]->name;
    if (params[0] != nullptr) {
        //read from the previous pipe
        //redirect stdout into the write-end of the next pipe
        dup2(pipe[0], STDIN_FILENO);
    }
    else {
        fprintf(stderr, "Error: output of '%s' can't be piped\n", cmdTable[index]->name);
        exit(1);
    }
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
    if (cmdTable.size() > 1) {
        for (int i = 0; i < cmdTable.size() - 1; i++) {
            int* p = new int[2]; //memleak
            //int p[2];
            if (pipe(p) == -1) {
                perror("Pipe");
                return 1;
            }
            pipes.push_back(p);
        }
    }

    //keeps track of pid
    std::vector<pid_t> children;

    //parent forks for each general command in command line
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

        //certain built-ins can't be run in background
        //child processes have different memory spaces than parent, so running 'cd', 'setenv', etc. would be pointless
        if (cmdTable[i]->filepath == nullptr && cmdTable.size() == 1 && cmdTable[i]->input == nullptr && cmdTable[i]->output == nullptr) {
            //want alias and printenv to redirect their stdout to a file/pipe using a process
            return builtIn(params, size, i);
        }

        //general commands and select built-in can be run in the background (by child processes)
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error: fork failed for command '%s'\n", name);
            return 1;
        }
        else if (pid == 0) {
            if (cmdTable.size() == 1) { //no pipes
                //setup function sets up IO redirection and handles built-in commands
                setupOnly(params, size, i);
            }
            else if (i == 0) { //first command in pipe
                setupFirst(params, size, i, pipes[i]);
            }
            else if (i > 0 && i < cmdTable.size() - 1) { //middle of pipe
                setupMiddle(params, i, pipes[i - 1], pipes[i]);
            }
            else { //end of pipe
                setupLast(params, i, pipes[i - 1]);
            }

            //after setting up IO and pipes, can now execute
            //close child's copies of ALL the pipes (pipe file descriptors for process no longer needed)
            for (int j = 0; j < pipes.size(); j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            //if filepath == nullptr, then a built-in executed 
            if (params[0] != nullptr) {
                execv(params[0], params); //exec only returns if there is an error
                perror("Execv");
                exit(1);
            }
            exit(0); //exec already returns on success for general commands, need to return for built-in
        }
        children.push_back(pid);
    }

    //close parent's copies of all the pipes
    for (int j = 0; j < pipes.size(); j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
    }    

    //after creating children, wait if needed
    //wait() suspends execution until a child terminates (since pipes are blocking, children complete by order in pipe)
    if (background == false) {
        for (int i = 0; i < children.size(); i++) {
            int status;
            wait(&status); //equivalent to waitpid(-1, &status, 0);

            //check if process exited normally
            if(WIFEXITED(status)) {
                //if process failed and exited normally, pipe broke, want to stop execution of rest of pipe
                if(WEXITSTATUS(status) != 0) {
                    for (int j = 0; j < children.size(); j++) {
                        kill(children[j], SIGKILL);
                    }
                    return 1;
                }
            }//killed processes will be exited abnormally and their status is ignored
        }
    }
    return 0;
}

void shell_init() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    setenv("PATH", ".:/bin", 1);
    setenv("HOME", cwd, 1);
    //setenv("PWD", cwd, 1);
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
            printf("\n");
            if (processCommand() == 0) {
                if (endShell) {
                    printf("Successfully exited shell\n");
                    break;
                }
            }
        }
        else {
            yyrestart(yyin);
        }
        //parser error already handled in .y

        //testCommandTable();
        cleanUp();
    }
    freeMem();

   return 0;
}