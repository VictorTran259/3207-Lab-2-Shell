#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"
#include "builtins.h"
#include "helpers.h"

//Redirects the standard output of a process to a file (>).
void redirectToSTDOUT(char **commandLine, int i) {
    int outputfile;

    //open the output file. file is created if it doesn't exist and deletes everything in the file if it does exist.
    outputfile = open(commandLine[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if(outputfile == -1) {
        printf("failed to open file.\n");
    }
    else {
        //change STDOUT to the output file.
        dup2(outputfile, STDOUT_FILENO);
        close(outputfile);
    }
}

//Redirects the contents of a file to a process's standard input (<).
void redirectToSTDIN(char **commandLine, int i) {
    int inputfile;

    //open the input file to be read.
    inputfile = open(commandLine[i+1], O_RDONLY, 0777);
    if(inputfile == -1) {
        printf("failed to open file.\n");
    }
    else {
        //change STDIN to the input file.
        dup2(inputfile, STDIN_FILENO);
        close(inputfile);
    }  
}

//executes a piping command. takes in the numPipes which is how many pipes were in the command line found by checkForPiping and ampersand
//flag to use with the wait built-in command.
//**DOES NOT WORK WITH MULTIPLE PIPES. I COULDN'T FIGURE OUT HOW TO DO IT SO I GAVE UP. IT WORKS WITH ONLY ONE PIPE THOUGH.**
void piping(char **commandLine, int numPipes, int wasThereAnAmpersand) {
    //An array of file descriptors for each program.
    int fd[numPipes][2];

    //Flag that keeps track of whether all the pipes created correctly.
    //0 = all good, 1 = a pipe failed to create
    int didallthepipescreatecorrectly;

    //Create numPipes pipes. If one of the pipes fails to create for some reason,
    //print an error message and close any pipes that were created before that pipe
    //to prevent anything bad from happening.
    int i = 0;
    while(i < numPipes) {
        //A pipe failed to create if this if condition is met.
        if(didallthepipescreatecorrectly = pipe(fd[i]) == -1) {
            printf("pipe %d failed to create.\n", i);

            //Close all the previously created pipes if there are any.
            for(int j = i; j >= 0; j--) {
                for(int k = 0; k < 2; k++) {
                    close(fd[j][k]);
                }
            }
            
            //Stop creating pipes.
            break;
        }
        //Creation of this pipe went fine. Keep creating more pipes.
        else {
            i++;
        }
    }

    //variable to keep track of the position in the commandLine array.
    int positionIncommandLine = 0;

    //Check to make sure all the pipes created correctly before executing.
    //No execution will occur if a pipe failed to create.
    if(didallthepipescreatecorrectly == 0) {
        for(int i = 0; i < numPipes; i++) {
            //build an array for the arguments of the first command.
            //**BASED ON THE CODE FROM helpers.c**
            char **args1 = malloc(sizeof(char*));
            *args1 = NULL;
            int n = 0;

            int j = 0;
            while(strcmp(commandLine[positionIncommandLine], "|") != 0 && commandLine[positionIncommandLine] != NULL) {
                args1 = realloc(args1, (n+2)*sizeof(char*));
                args1[j] = commandLine[positionIncommandLine];
                positionIncommandLine++;
                j++;
            }
            
            if(strcmp(commandLine[positionIncommandLine], "|") == 0) {
                positionIncommandLine++;
            }

            args1[j] = NULL;

            int resetposition = positionIncommandLine;

            //build an array for the arguments of the second command.
            //**BASED ON THE CODE FROM helpers.c**
            char **args2 = malloc(sizeof(char*));
            *args2 = NULL;
            int m = 0;

            int k = 0;
            //the if statement is to prevent segmentation faulting.
            if(commandLine[positionIncommandLine] != NULL) {
                while(strcmp(commandLine[positionIncommandLine], "|") != 0 && commandLine[positionIncommandLine] != NULL) {
                    args2 = realloc(args2, (m+2)*sizeof(char*));
                    args2[k] = commandLine[positionIncommandLine];
                    positionIncommandLine++;
                    k++;
                    if(commandLine[positionIncommandLine] == NULL) {
                        break;
                    }
                }
            }

            args2[k] = NULL;
            
            positionIncommandLine = resetposition;

            //The PATH scanner from shell.c used to find the path to the first command. 
            char *directoryName1 = NULL;
            char pathToCommand1[LINE_MAX] = { 0 };

            //Try to locate the program in one of the paths in "PATH".
            if(*args1[0] != '.' && *args1[0] != '/') {
                //Copy the contents of "PATH" into a buffer.
                char *environment1 = getenv("PATH");
                char buf1[LINE_MAX];
                strcpy(buf1, environment1);

                char *environment2 = getenv("PATH");
                char buf2[LINE_MAX];
                strcpy(buf2, environment1);

                //Flag to indicate whether the specified program is located in the current path in "PATH".
                //0 = program is not in this path of "PATH", 1 = program is in this path of "PATH".
                int istheprograminthisdir1 = 0;
                int istheprograminthisdir2 = 0;

                //call strtok to set directoryName the the first path of "PATH".
                directoryName1 = strtok(buf1, ":");

                //Scan through all the paths of "PATH" and check which path the program is located in.
                while(directoryName1 != NULL) {
                    //istheprograminthisdir will be set to 1 by scandirectory if it finds the program 
                    //in the current path specified by directoryName.
                    istheprograminthisdir1 = scandirectory(directoryName1, args1);

                    //Case: the program was not found in this path so strtok to the next path in "PATH".
                    if(istheprograminthisdir1 == 0) {
                        directoryName1 = strtok(NULL, ":");
                    }
                    //Case: the program was found in this path so no need to check the other paths in "PATH".
                    else {
                        break;
                    }
                }

                //Build a path to the program to the buffer pathToCommand if the program was found in a directory (directoryName is not NULL).
                if(directoryName1 != NULL) {
                    strcat(pathToCommand1, directoryName1);
                    strcat(pathToCommand1, "/");
                    strcat(pathToCommand1, args1[0]);
                }
                else {
                    printf("%s: command does not exist.\n", args1[0]);
                }
            }
            
            //The PATH scanner from shell.c used to find the path to the second command. 
            char *directoryName2 = NULL;
            char pathToCommand2[LINE_MAX] = { 0 };

            //Try to locate the program in one of the paths in "PATH".
            if(*args2[0] != '.' && *args2[0] != '/') {
                //Copy the contents of "PATH" into a buffer.
                char *environment2 = getenv("PATH");
                char buf2[LINE_MAX];
                strcpy(buf2, environment2);

                //Flag to indicate whether the specified program is located in the current path in "PATH".
                //0 = program is not in this path of "PATH", 1 = program is in this path of "PATH".
                int istheprograminthisdir2 = 0;

                //call strtok to set directoryName the the first path of "PATH".
                directoryName2 = strtok(buf2, ":");

                //Scan through all the paths of "PATH" and check which path the program is located in.
                while(directoryName2 != NULL) {
                    //istheprograminthisdir will be set to 1 by scandirectory if it finds the program 
                    //in the current path specified by directoryName.
                    istheprograminthisdir2 = scandirectory(directoryName2, args2);

                    //Case: the program was not found in this path so strtok to the next path in "PATH".
                    if(istheprograminthisdir2 == 0) {
                        directoryName2 = strtok(NULL, ":");
                    }
                    //Case: the program was found in this path so no need to check the other paths in "PATH".
                    else {
                        break;
                    }
                }

                //Build a path to the program to the buffer pathToCommand if the program was found in a directory (directoryName is not NULL).
                if(directoryName2 != NULL) {
                    strcat(pathToCommand2, directoryName2);
                    strcat(pathToCommand2, "/");
                    strcat(pathToCommand2, args2[0]);
                }
                else {
                    printf("%s: command does not exist.\n", args2[0]);
                }
            }

            pid_t pid1 = fork();

            //throw an error if fork failed.
            if(pid1 == -1) {
                printf("failed to fork.\n");
            }

            if(pid1 == 0) {
                if(i == numPipes-1) {
                    dup2(fd[i][1], STDOUT_FILENO);
                }
                //couldn't figure this out.
                else {
                    dup2(fd[i][1], 1); //?
                }
                //close all the unused pipes.
                for(int i = 0; i < numPipes; i++) {
                    for(int j = 0; j < 2; j++) {
                        close(fd[i][j]);
                    }
                }

                //Flag that indicates whether the exec worked or not.
                //-1 = exec failed, no return = exec worked.
                int diditexec = 0;

                //Case: the command isn't something like "./program".
                //Exec the path to the command that was found by the if statement above.
                if(directoryName1 != NULL) {
                    diditexec = execv(pathToCommand1, args1);
                }
                //Case: the command is something like "./program" or "/bin/program".
                //Just execs the specified program.
                else {
                    diditexec = execv(args1[0], args1);
                }

                //diditexec = -1 means the exec failed for some reason. Print out an error message.
                if(diditexec == -1) {
                    printf("exec failed.\n");
                }
                exit(0);
            }
            else {
                pid_t pid2 = fork();

                //throw an error if fork failed.
                if(pid2 == -1) {
                    printf("failed to fork.\n");
                }

                if(pid2 == 0) {
                    if(i == 0) {
                        dup2(fd[i][0], STDIN_FILENO);
                    }
                    //couldn't figure this out.
                    else {
                        dup2(fd[i][0], 0); //?
                    }
                    //close all the unused pipes
                    for(int i = 0; i < numPipes; i++) {
                        for(int j = 0; j < 2; j++) {
                            close(fd[i][j]);
                        }
                    }
                    
                    //Flag that indicates whether the exec worked or not.
                    //-1 = exec failed, no return = exec worked.
                    int diditexec = 0;

                    //Case: the command isn't something like "./program".
                    //Exec the path to the command that was found by the if statement above.
                    if(directoryName2 != NULL) {
                        diditexec = execv(pathToCommand2, args2);
                    }
                    //Case: the command is something like "./program" or "/bin/program".
                    //Just execs the specified program.
                    else {
                        diditexec = execv(args2[0], args2);
                    }

                    //diditexec = -1 means the exec failed for some reason. Print out an error message.
                    if(diditexec == -1) {
                        printf("exec failed.\n");
                    }
                    exit(0);
                }

                //Close all the pipes after execution is done.
                for(int i = 0; i < numPipes; i++) {
                    for(int j = 0; j < 2; j++) {
                        close(fd[i][j]);
                    }
                }

                waitforchild(pid1, wasThereAnAmpersand);
                waitforchild(pid2, wasThereAnAmpersand);
            }

            free(args1);
            free(args2);
        }
    }
}

