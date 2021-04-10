https://stackoverflow.com/questions/33884291/pipes-dup2-and-exec
https://stackoverflow.com/questions/23097893/how-do-you-create-unix-pipes-dynamically
https://stackoverflow.com/questions/6820450/pipes-between-child-processes-in-c

processCommand() {

    vector<pipefd[]> pipes

    //check if command exists
    //check if command is accessible
    //check if command can have io redirection (for built-in)
    //check if file you are input or output redirecting exists
    for command in command table {
        //if check not met, throw error 
    }

    //establish pipes if needed
    if cmdTable.size() > 1 {
        for command in command table { //want to create n - 1 pipes
            pipefd[2]
            pipe(pipefd)
            vector.pushback(pipefd)
        }   
    }

    execute(pipes) {

        //where to store output built-in returns and writes to pipe
        buff[]

        //where to store input built-in recieves from a pipe
        buff2[]

        //NEED TO FORK FOR EVERY COMMAND IN COMMAND TABLE
        for command in command table {

            //do checks here?

            fork
            if child { 

                //SETUP FILEDESCRIPTORS AND PIPING BEFORE EXECUTION
                if IO and no piping {
                    //set file descriptors
                }
                if there is piping {
                    //check position in pipe
                    if i == 1 { //only need to write to process
                        if input redirection {
                            //set file descriptor for input
                        }

                        pipefd = pipes[i];
                        if not built in {
                            dup2(pipefd[WRITE_END], STDOUT)
                            close(fd[WRITE_END])
                            close(fd[READ_END])
                        }
                        else if built in {
                            write(pipefd[WRITE_END], buff, sizeof(buff));
                        }
                    }
                    else { //need to read and write
                        pipefd1 = pipes[i - 1];
                        pipefd2 = pipes[i];

                        if not built in {
                            dup2(pipefd1[READ_END], STDIN)
                            close(pipefd1[WRITE_END])
                            close(pipefd1[READ_END])

                            dup2(pipefd2[WRITE_END], STDOUT)
                            close(pipefd2[WRITE_END])
                            close(pipefd2[READ_END])
                        }
                        else if built in {
                            //have return of built-in go to buff
                            //have input needed for built-in (parameters?) go to buff2
                            write(pipefd1[WRITE_END], buff, sizeof(buff));
                            read(pipefd2[READ_END], buff2, sizeof(buff));
                        }
                    }
                    else if i == n { //need to only read
                        if output redirection {
                            //set file descriptor for input
                        }

                        pipefd = pipes[i];

                        if not built in {
                            dup2(pipefd[READ_END], STDIN)
                            close(fd[WRITE_END])
                            close(fd[READ_END])
                        }
                        else if built in {
                            read(pipefd[READ_END], buff2, sizeof(buff));
                        }
                    }
                }

                //EXECUTION AFTER SETTING UP PIPING AND FILE DESCRIPTORS
                //check if it's a built in, and if so execute built in
                //otherwise do execv

            }
            else if parent { 
                if execute in background == false { //change to external bool
                    wait(NULL);
                }
            }
        }

        return;
    }
}