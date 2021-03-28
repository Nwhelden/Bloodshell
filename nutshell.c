#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <pwd.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void printPrompt() {
    char cwd[PATH_MAX+1];
    char host[HOST_NAME_MAX+1];
    char login[LOGIN_NAME_MAX+1];
    struct passwd *p = getpwuid(getuid());
    getcwd(cwd, sizeof(cwd));
    gethostname(host, sizeof(host));
    printf(ANSI_COLOR_RED "%s@%s~%s>" ANSI_COLOR_RESET, p->pw_name, host, cwd);
}

int yyparse();
int yylex();
extern FILE* yyin;
extern void yyrestart();

int main() {

    while(1) {
        printPrompt();
        yyparse();
        yyrestart(yyin);
    }
   return 0;
}