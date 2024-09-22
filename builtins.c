#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "helpers.h"

//help command. when called, prints out the manual for the shell located in the file "help.txt".
//must be in the same directory where help.txt is located.
void help() {
    FILE *file;
    size_t length = 0;
    char *line = NULL;

    file = fopen("help.txt", "r");

    //Throw an error if help.txt was unable to be opened.
    if(file == NULL) {
        printf("Failed to open help.txt");
    }
    //Print out each line of the help.txt file.
    else {
        while(getline(&line, &length, file) != -1) {
            printf("%s", line);
        }
    }

    printf("\n");
}

//exit command. when called, it exits the user from the shell.
void exitshell() {
    printf("Closing Shell...\n");
    exit(0);
}

//pwd command. when called, prints out the full path name of the user's current working directory
void printworkingdirectory() {
    char cwd[LINE_MAX];

    getcwd(cwd, sizeof(cwd));
    printf("%s\n", cwd);
}

//cd command. when called, changes to a different current working directory.
void changedirectory(char **commandLine) {
    int test = chdir(commandLine[1]);
    if(test == -1) {
        printf("The directory you entered is invalid.\n");
    }
    else{
        printf("Successfully changed directory.\n");
    }
}

//wait command. when called by a function, if there was an ampersand indicated by the ampersand flag, the shell will not wait for the process to finish execution
//before the user can enter the next command.
void waitforchild(pid_t pid, int wasThereAnAmpersand) {
    if(wasThereAnAmpersand == 1) {
        waitpid(pid, NULL, WNOHANG);
    }
    else {
        waitpid(pid, NULL, 0);
    }
}