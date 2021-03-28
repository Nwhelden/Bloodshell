%{
    #include <stdio.h>
    int yylex();
    void yyerror(char* s){ 
        fprintf(stderr, "%s\n", s);
    };
%}

%define api.value.type union 

%token <char> CHARACTER
%token UNDEFINED

%%

character: CHARACTER {printf("charater: %c\n", yylval.CHARACTER); return 0;}

%%