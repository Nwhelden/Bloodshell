%{
    #include <stdio.h>
    #include <string.h>
    #include "command.h"
    int yylex();
    extern std::vector<Command*> cmdTable;
    void yyerror(const char* s);
    void addCommand(char* name);
    extern bool background;
    extern int yylineno;
    extern char* yytext;
    extern bool endShell;
%}

%define api.value.type union 

%token <char*> WORD
%token UNDEFINED NEWLINE ERRDIR ERRDIR2 END_OF_FILE

%%

line: NEWLINE { return 0; }
    | cmd NEWLINE { return 0; }
    | cmd { return 0; }
    | END_OF_FILE { endShell = true; return 0; }
    ;

cmd: name parameters stdin_redirection out_redirection background {;}
   ;

cmd2: name parameters out_redirection {;}
   ;

name: WORD { addCommand($1); }
;

parameters: %empty {;}
         | WORD parameters { cmdTable.back()->parameters.push_back($1); }
         ;

stdin_redirection: %empty {;}
                 | '<' WORD { cmdTable.back()->input = $2; }
                 ;

out_redirection: '|' cmd2 {;}
               | stdout_redirection stderr_redirection {;}


stdout_redirection: %empty {;}
                  | '>' WORD { cmdTable.back()->output = $2; }
                  | '>' '>' WORD { cmdTable.back()->output = $3; cmdTable.back()->outAppend = true; }
                  ;

stderr_redirection: %empty {;}
                  | ERRDIR WORD { cmdTable.back()->err = $2; }
                  | ERRDIR2 { cmdTable.back()->errToOut = true; }
                  ;

background: %empty {;}
          | '&' { background = true; }
          ;

%%

//called if a token scanned by yylex has no associated rule
void yyerror(const char* s){ 
    //fprintf(stderr, "%s\n", s);
    fprintf(stderr, "Syntax error [line %d]: '%s' unrecognized\n", yylineno, yytext);
};

Command::Command(char* name): builtIn { false }, outAppend { false }, errToOut { false }, input{ nullptr }, output{ nullptr }, err { nullptr }, 
filepath { nullptr } {
    this->name = name;
};

void addCommand(char* name) {
    Command* ptr = new Command(name);
    cmdTable.push_back(ptr);
}
