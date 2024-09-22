#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include "builtins.h"
#include "helpers.h"
#include "redirectionandpiping.h"
#include "shell.h"

int main(int argc, char **argv) {
    //Buffer that holds the user's current working directory.
    char cwd[LINE_MAX];

    //The while loop (shell) will continuously execute unless the user calls exit.
    while(1) {
        //Shell formatting. Call getcwd() to copy the current working directory to the buffer cwd
        //and then print out the name of the shell along with the current working directory path
        //just like how the tcsh shell does every time before the user executes a command.
        getcwd(cwd, sizeof(cwd));
        printf("MyShell:~%s>", cwd);

        //***I JUST COPIED THE FORMAT OF THE CODE THAT WAS PROVIDED IN THE MAIN FUNCTION OF helpers.c***
        //Parses the command line the user entered into the shell into the commandLine buffer.
        char *line;
        size_t size = 0;
        getline(&line, &size, stdin);

        char **commandLine = parse(line," \n");

        //First, the shell will check to see if the command the user entered was a built-in command.
        //The variable wasitabuiltin is a flag indicating whether the command the user entered was a built-in command.
        //0 = the command was not a built-in, 1 = the command was a built-in.
        int wasitabuiltin = checkIfBuiltIn(commandLine);

        if(wasitabuiltin != 1) {
            //Next, the shell will check if the user entered an ampersand.
            //The variable wasThereAnAmpersand is a flag indicating whether the user entered an ampersand.
            int wasThereAnAmpersand = checkForAmpersand(commandLine);

            //NOTE: This shell doesn't have to support both piping and redirection so this setup below is fine.
            //ex. "ls | grep shell > output.txt" 
            //The shell will break if someone tries to do both piping and redirection in one command line.
            int wasthereredirection = checkForRedirection(commandLine, wasThereAnAmpersand);
            int wastherepiping = checkForPipes(commandLine, wasThereAnAmpersand);

            //no special cases were found so just execute the program in the command line.
            if(wasthereredirection != 1 && wastherepiping != 1) {
                execute(commandLine, wasThereAnAmpersand);
            }
        }

        free(commandLine);
        free(line);
    }
}

int checkIfBuiltIn(char **commandLine) {
    //Count how many arguments the user entered and store it in argcounter
    int argcounter = 0;

    int i = 0;
    while(commandLine[i] != NULL) {
        argcounter++;
        i++;
    }

    //Flag indicating whether the command was a built-in or not.
    //0 = it was not a built-in, 1 = it was a built-in.
    int wasitabuiltin = 0;

    //These four sets of if and else if statements check the command to see if it was one of
    //the built-ins (exit, pwd, help, and cd). An error message is printed out if the amount of
    //arguments found in the command line is not appropriate for the command.
    if(strcmp(commandLine[0], "exit") == 0 && argcounter == 1) {
        exitshell();
        wasitabuiltin = 1;
    }
    else if(strcmp(commandLine[0], "exit") == 0 && argcounter > 1) {
        printf("Too many arguments for exit.\n");
        wasitabuiltin = 1;
    }

    if(strcmp(commandLine[0], "pwd") == 0 && argcounter == 1) {
        printworkingdirectory();
        wasitabuiltin = 1;
    }
    else if(strcmp(commandLine[0], "pwd") == 0 && argcounter > 1) {
        printf("Too many arguments for pwd.\n");
        wasitabuiltin = 1;
    }

    if(strcmp(commandLine[0], "help") == 0 && argcounter == 1) {
        help();
        wasitabuiltin = 1;
    }
    else if(strcmp(commandLine[0], "help") == 0 && argcounter > 1) {
        printf("Too many arguments for help.\n");
        wasitabuiltin = 1;
    }

    if(strcmp(commandLine[0], "cd") == 0 && argcounter == 2) {
        changedirectory(commandLine);
        wasitabuiltin = 1;
    }
    else if(strcmp(commandLine[0], "cd") == 0 && argcounter == 1) {
        printf("Too few arguments for cd.\n");
        wasitabuiltin = 1;
    }
    else if(strcmp(commandLine[0], "cd") == 0 && argcounter > 2) {
        printf("Too many arguments for cd.\n");
        wasitabuiltin = 1;
    }

    return wasitabuiltin;
}

//Checks the last argument of the command line for an ampersand.
int checkForAmpersand(char **commandLine) {
    //Count how many arguments the user entered and store it in argcounter
    int argcounter = 0;

    int i = 0;
    while(commandLine[i] != NULL) {
        argcounter++;
        i++;
    }

    //If there's an ampersand as the last argument of the command line, set this flag below to 1.
    //0 = there was no ampersand, 1 = there was an ampersand.
    int wasThereAnAmpersand = 0;

    //Check the last argument of the command line to see if there was an ampersand.
    //If there is, the flag wasThereAnAmpersand gets set to 1 and the ampersand argument
    //gets set to NULL in the array so it doesn't cause any problems during execution later.
    if(strcmp(commandLine[argcounter-1], "&") == 0) {
        wasThereAnAmpersand = 1;
        commandLine[argcounter-1] = NULL;
    }

    return wasThereAnAmpersand;
}

//checks the command line for redirection. takes in the ampersand flag for use in execution later.
int checkForRedirection(char **commandLine, int wasThereAnAmpersand) {
    //Flag that indicates whether there is redireciton in the command line (">" or "<").
    //0 = there was no redirection, 1 = there was redirection
    int isThereRedirection = 0;

    //Hold the original file descriptors for the purpose of resetting the file descriptors back to normal after each execution.
    int originalSTDOUT = dup(STDOUT_FILENO);
    int originalSTDIN = dup(STDIN_FILENO);

    //Check for redirection symbols in the command line and redirect to the correct spot if there is.
    int i = 0;

    //Flags used to check for "command < file < file" and "command > file > file" which are invalid.
    int wasthelastredirectSTDOUT = -1;
    int wasthelastredirectSTDIN = -1;
    int throwerror = 0;

    //check the command line for < or >.
    while(commandLine[i] != NULL) {
        if(strcmp(commandLine[i], ">") == 0) {
            if(wasthelastredirectSTDOUT == 1) {
                throwerror = 1;
                break;
            }
            wasthelastredirectSTDOUT = 1;
            wasthelastredirectSTDIN = 0;

            isThereRedirection = 1;
            redirectToSTDOUT(commandLine, i);
        }
        if(strcmp(commandLine[i], "<") == 0) {
            if(wasthelastredirectSTDIN == 1) {
                throwerror = 1;
                break;
            }
            wasthelastredirectSTDOUT = 0;
            wasthelastredirectSTDIN = 1;

            isThereRedirection = 1;
            redirectToSTDIN(commandLine, i);
        }
        i++;
    }

    //If the isThereRedirection flag is set to 1 (there was redirection), execute the program here in this function with
    //the redirection before the file descriptors get reset back to normal at the end of this function.
    if(isThereRedirection == 1 && throwerror != 1) {
        //Pass a new char array without all the unneccessary stuff at the end to the execute function which would mess up the execute.
        //***THIS IS BASED ON THE CODE PROVIDED IN THE helpers.c FILE.***
        char **args = malloc(sizeof(char*));
        *args = NULL;
        int n = 0;

        int i = 0;
        while(strcmp(commandLine[i], ">") != 0 && strcmp(commandLine[i], "<") != 0  && commandLine[i] != NULL) {
            args = realloc(args, (n+2)*sizeof(char*));
            args[i] = commandLine[i];
            i++;
        }

        args[i] = NULL;

        execute(args, wasThereAnAmpersand);

        free(args);
    }

    //Reset the file descriptors back to normal for future commands.
    dup2(originalSTDOUT, STDOUT_FILENO);
    dup2(originalSTDIN, STDIN_FILENO);
    close(originalSTDIN);
    close(originalSTDOUT);

    //Throw an error if the flag was set to 1.
    if(throwerror == 1) {
        printf("error: you entered \"command < file < file\" or \"command > file > file\" which are are invalid.\n");
    }

    return isThereRedirection;
}

//check the command line for any pipes. if there were pipes, call piping to execute the piping. 
int checkForPipes(char **commandLine, int wasThereAnAmpersand) {
    //flag to check if a pipe was present in the command line
    int isThereAPipe = 0;
    //counter to keep track of how many pipes were present.
    int pipeCounter = 0;

    //look through the command line to check for "|".
    int i = 0;
    while(commandLine[i] != NULL) {
        if(strcmp(commandLine[i], "|") == 0) {
           isThereAPipe = 1;
           pipeCounter++;
        }
        i++;
    }

    //call piping to execute if there was a pipe.
    if(isThereAPipe == 1) {
        piping(commandLine, pipeCounter, wasThereAnAmpersand);
    }

    return isThereAPipe;
}

//Executes a program that isn't one of the built ins. the ampersand flag is used here when the built-in wait is called.
void execute(char **commandLine, int wasThereAnAmpersand) {
    char *directoryName = NULL;
    char pathToCommand[LINE_MAX] = { 0 };

    //Case: the command isn't something like "./program" or "/bin/program".
    //Try to locate the program in one of the paths in "PATH".
    if(*commandLine[0] != '.' && *commandLine[0] != '/') {
        //Copy the contents of "PATH" into a buffer.
        char *environment = getenv("PATH");
        char buf[LINE_MAX];
        strcpy(buf, environment);

        //Flag to indicate whether the specified program is located in the current path in "PATH".
        //0 = program is not in this path of "PATH", 1 = program is in this path of "PATH".
        int istheprograminthisdir = 0;

        //call strtok to set directoryName the the first path of "PATH".
        directoryName = strtok(buf, ":");

        //Scan through all the paths of "PATH" and check which path the program is located in.
        while(directoryName != NULL) {
            //istheprograminthisdir will be set to 1 by scandirectory if it finds the program 
            //in the current path specified by directoryName.
            istheprograminthisdir = scandirectory(directoryName, commandLine);

            //Case: the program was not found in this path so strtok to the next path in "PATH".
            if(istheprograminthisdir == 0) {
                directoryName = strtok(NULL, ":");
            }
            //Case: the program was found in this path so no need to check the other paths in "PATH".
            else {
                break;
            }
        }

        //Build a path to the program to the buffer pathToCommand if the program was found in a directory (directoryName is not NULL).
        if(directoryName != NULL) {
            strcat(pathToCommand, directoryName);
            strcat(pathToCommand, "/");
            strcat(pathToCommand, commandLine[0]);
        }
        else {
            printf("%s: command does not exist.\n", commandLine[0]);
        }
    }

    //Fork in order to prepare the shell to exec the program.
    pid_t pid = fork();

    //pid = -1 means the fork failed for some reason. Print out an error message.
    if(pid == -1) {
        printf("fork failed.\n");
    }

    //The shell is in the child process if pid = 0 so execution of the program happens here.
    if(pid == 0) {
        //Flag that indicates whether the exec worked or not.
        //-1 = exec failed, no return = exec worked.
        int diditexec = 0;

        //Case: the command isn't something like "./program".
        //Exec the path to the command that was found by the if statement above.
        if(directoryName != NULL) {
            diditexec = execv(pathToCommand, commandLine);
        }
        //Case: the command is something like "./program" or "/bin/program".
        //Just execs the specified program.
        else {
            diditexec = execv(commandLine[0], commandLine);
        }

        //diditexec = -1 means the exec failed for some reason. Print out an error message.
        if(diditexec == -1) {
            printf("exec failed.\n");
        }
        exit(0);
    }
    //The shell is in the parent process if pid does not equal 0 or -1 so make it wait.
    else {
        waitforchild(pid, wasThereAnAmpersand);
    }
}

//***THIS IS CODE THAT I BASICALLY COPIED FROM MY PROJECT 0 PART A (tuls.c)***
//Scans through a path in "PATH" and tries to locate the program for the command specified by the user.
int scandirectory(char *pathName, char **commandLine) {
    //Flag to indicate whether the program was found in the directory. 0 = not found, 1 = found.
    int wastheprogramfound = 0;

    //Open the directory
    DIR *d = opendir(pathName);

    //Case: the shell could not open the directory. The shell will just do nothing then.
    if(d == NULL) {
    }
    //Case: the shell could open the directory. The shell will scan through the directory looking for the specified program.
    //The variable wastheprogramfound will be set to 1 if the specified program was found
    else {
        struct dirent *dir;

        //Scan through the contents of this path and check for the specified program.
        while((dir = readdir(d)) != NULL && wastheprogramfound == 0) {
            //The program was found if the conditon for this if statement was met.
            //Set wastheprogramfound = 1 to indicate this.
            if(strcmp(commandLine[0], dir->d_name) == 0) {
                wastheprogramfound = 1;
            }
        }

        closedir(d);
    }

    return wastheprogramfound;
}