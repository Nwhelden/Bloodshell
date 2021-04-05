%{
    #include <stdio.h>
    #include "command.h"
    int yylex();
    extern std::vector<Command*> cmdTable;
    void yyerror(const char* s);
    void addCommand(char* name);
%}

%define parse.error verbose

%define api.value.type union 

%token <char*> WORD ENV
%token UNDEFINED NEWLINE

%%

line: NEWLINE { return 0; }
    | ENV NEWLINE { printf("%s\n", $1); return 0; }
    | cmd NEWLINE { return 0; }
    ;

cmd: name parameters stdin_redirection stdout_redirection background {;}
   ;

name: WORD { addCommand($1); }
;

parameters: %empty {;}
         | WORD parameters { cmdTable.back()->parameters.push_back($1); }
         ;

stdin_redirection: %empty {;}
                 | '<' WORD { cmdTable.back()->input = $2; }
                 ;

stdout_redirection: %empty {;}
                  | '>' WORD { cmdTable.back()->output = $2; }
                  | '|' cmd {;}
                  ;

background: %empty { cmdTable.back()->background = false; }
          | '&' { cmdTable.back()->background = true; }
          ;

%%

//called if a token scanned by yylex has no associated rule
void yyerror(const char* s){ 
    fprintf(stderr, "%s\n", s);
};

Command::Command(char* name): input{ nullptr }, output{ nullptr } {
    this->name = name;
};

void addCommand(char* name) {
    Command* ptr = new Command(name);
    cmdTable.push_back(ptr);
}
