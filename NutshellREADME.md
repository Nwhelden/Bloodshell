# README: Nutshell Term Project

# About: 
In this project, we created our own shell from scratch can accept linux commands and translate them into actual OS instructions. To complete this project, we used the tokenizers lex and yacc as well as the C++ programming language.

# Features we did not implement:
We did not implement the following features:
1. Wildcard matching
2. Tilde expansion(Extra Credit)
3. File Name Completion(Extra Credit)

# Features Implemneted/Role Distribution:
We implemented the following features:
1. Lexer file (Nathan)
2. Parser file (Nathan)
3. Built-in Commands (Juan)
4. Non built-in Commands (Juan)
5. Check if non built-in commands had implementations in PATH directories (Juan)
6. Redirecting I/O with non built-in Commands (Juan)
7. Using Pipes with non built-in Commands (Juan/Nathan)
8. Running non built-in Commands in background (Juan/Nathan)
9. Using Pipes and I/O redirection combined, with non built-in Commands (Juan/Nathan)
10. Environment Variable Expansion (Nathan)
11. Alias Expansion (Nathan)
12. Error handling (Juan/Nathan)

# How to run code:
Run the Makefile in order to compile all the necessary files to execute the shell. CD into the parent directory which has all the nutshell.cpp, lexer.l, and parser.y files, then run the nutshell executable file to use the shell and test commands.