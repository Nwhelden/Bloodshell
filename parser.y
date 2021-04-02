%{
    #include <stdio.h>
    #include "command.h"
    int yylex();
    extern std::vector<Command*> cmdTable;
    void yyerror(char* s);
    void addCommand(char* name);
%}

%define api.value.type union 

%token <char*> WORD NAME
%token UNDEFINED NEWLINE

%%

line: cmd NEWLINE { return 0; }
    ;

cmd: name parameters stdin_redirection stdout_redirection {;}
   ;

name: NAME { addCommand($1); }
    ;

parameters: %empty {;}
         | WORD parameters { cmdTable.back()->parameters.push_back($1); }
         | NAME parameters { cmdTable.back()->parameters.push_back($1); }
         ;

stdin_redirection: %empty {;}
                 | '<' NAME { cmdTable.back()->input = $2; }
                 ;

stdout_redirection: %empty {;}
                  | '>' NAME { cmdTable.back()->output = $2; }
                  | '|' cmd {;}
                  ;

%%

void yyerror(char* s){ 
    fprintf(stderr, "%s\n", s);
};

Command::Command(char* name): input{ nullptr }, output{ nullptr } {
    this->name = name;
};

void addCommand(char* name) {
    Command* ptr = new Command(name);
    cmdTable.push_back(ptr);
}
