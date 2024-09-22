# CIS 3207 Lab 2 - Shell
## Project files

      builtins.c
      builtins.h
      helpers.c
      helpers.h
      redirectionandpiping.c
      redirectionandpiping.h
      shell.c
      shell.h
      help.txt
      pseudocode.txt
      makefile

## FUNCTIONS

### In builtins.c
    void help();
        Prints out the shell's manual that I put in the help.txt file when you type "help" into the shell and you're in the same directory it's located in.
        Done using fopen() and getline() in a while loop to print out each line of the manual.

    void exitshell();
        Exits you from the shell whenever you type "exit" into the shell. Done using a call to exit(0). Also prints out a message that you're exiting the shell
        when called.

    void printworkingdirectory();
        Print out the current working directory using getcwd() to read the current working directory into a buffer and then printing it out to the shell.
        You type "pwd" to the shell to call this function.

    void changedirectory(char **commandLine);
        Changes the current working directory to a new one specified by the user. Throws an error if the current working directory couldn't be changed or the
        user didn't enter a directory to change it to after "cd". Takes in the command line array to pass the specified directory to chdir()

    void waitforchild(pid_t pid, int wasThereAnAmpersand);
        Takes in a pid and a flag wasThereAnAmpersand that tells the shell whether the user entered an & at the end of the command line. If the user entered an &,
        the shell won't wait for the current running process run by the user to finish before letting the user enter another command. Otherwise, the shell will
        wait for the current running process run by the user to finish before allowing the user to enter another command.

### In helpers.c (This file was provided to me)
    char **parse(char*line,char*delim);
        I used this to parse the command line for the shell.

    int find_special (char*args[], char * special);
        I didn't use this.

    FILE *getInput(int argc, char* argv[]);
        I didn't use this.

### In redirectionandpiping.c
    void redirectToSTDOUT(char **commandLine, int i);
        Takes in the command line array and an int i which points to the location of a redirection ">" symbol in the command line array.
        Redirects the standard output of a process to the specified file using dup2() and open() with the write, create, and truncate flags.

    void redirectToSTDIN(char **commandLine, int i);
        Takes in the command line array and an int i which points to the location of a redirection "<" symbol in the command line array.
        Redirects the standard input of a process to the specified file using dup2() and open() with the read flag.

    void piping(char **commandLine, int numPipes, int wasThereAnAmpersand);
        Takes in the command line, number of pipes and the ampersand flag. First creates an array of pipes and then checks to see if they all opened properly.
        If a pipe failed to create, it throws an error message and closes all pipes created before it if any so no problems occur. If all the pipes were 
        created successfully, the function then goes into a for loop for each of the pipes present in the command line.
        The command line is then parsed using code I borrowed from the helpers.c file into two argument string arrays (args1 and args2) for two commands
        between a pipe (command1 | command2). Then the commands in args1 and args2 are both looked for by the function using code from execute() in shell.c.
        The shell forks twice and executes both programs using fork() and execv() but it changes the file descriptors of the pipes first.
        It uses the same executing method as the execute function in shell.c. The parent process waits for the two child processes to finish before closing
        all the pipes. I couldn't get the function to work with multiple pipes in the command line though.

### In shell.c
    main(int argc, char **argv);
        The main function holds the while(1) loop that simulates the shell. First, it prints out the current working directory every time before the user enters a command
        just like the tcsh shell does. It then parses the command line into an array for easier access to each argument the user entered. Then it checks to see if the user
        entered a built-in command using the checkIfBuiltIn function. If the command the user entered was not a built-in command, it then checks if the user entered an
        ampersand at the end of the command line for execution later. Then it checks for redirection and piping using the checkForRedirection and checkForPipes functions.
        If there was no redirection or piping, the shell will try to execute the command line the user entered. All of the checks are done using flags where 0 = not present
        and 1 = present.

    void execute(char **commandLine, int wasThereAnAmpersand);
        Takes in the command line array and the ampersand flag. If the program the user entered does not start with a "." or a "/", first checks through each path
        in PATH and tries to find the path where the program is located with the help of the scandirectory function. This is done using getenv(), strtok(), and a while loop
        that will keep going until all the paths in PATH have been looked through or the program was located in a path of PATH. If the program was located, build a path to it
        using strcat to a buffer. Then execute the program using fork() and execv(). If the path to the program was located in PATH, execute it using that path but if it wasn't,
        execute the program as it was specified in the command line array. Prints an error message if the fork or exec failed or if the command the user entered is invalid.
        ex. of invalid command: "l".

    int checkForPipes(char **commandLine, int wasThereAnAmpersand);
        Takes in the command line array and the ampersand flag for use in execution later. Scans through the arguments in the command line array and looks for pipes.
        Iterates a counter every time a pipe is found in the command line. If a pipe was found, call the piping function and pass it the command line, number of pipes counter,
        and the ampersand flag. Returns the isThereAPipe flag to main.

    int scandirectory(char *pathName, char **commandLine);
        Takes in one of the path names from PATH and also the command line array. Scans through the contents of the directory and looks for the specified program in the command
        line. This is done using a while loop and readdir(). The flag wastheprogramfound is returned specifying to the calling function whether the program was found or not.
        1 = present in the path, 0 = not present in the path.

    int checkIfBuiltIn(char **commandLine);
        Takes in the command line array that holds the command line the user entered and checks the first argument in it to see if it's one of the built-in commands.
        This is all done using a series of if and else if statements and an error will be printed out here if too many arguments for a specific built-in was entered.
        cd can have too little arguments as well and this function accounts for that case. Returns the flag wasitabuiltin to the main.

    int checkForRedirection(char **commandLine, int wasThereAnAmpersand);
        Takes in the command line array and the ampersand flag. First makes copies of the original standard input and standard output file descriptors for resetting them
        afterwards. Then check the command line array for redirection symbols using a while loop. If one is found, call the corresponding redirection function to redirect
        the standard input or standard output. Afterwards, create an string array called args that holds the arguments of the command the user entered without all the 
        redirection symbols and files for the purpose of execution. After this is done, the command in args is executed using the help of the execute function.
        The file descriptors are then reset at the end of the function and the flag isThereRedirection is returned to main. The function will print out an error message
        if the user tries to enter multiple of the same redirects back to back. ex. of this: "command > file1 > file2".

    int checkForAmpersand(char **commandLine);
        Takes in the command line array that holds the command line the user entered and checks the last argument of the array to see if the user entered an & as the final
        argument. First it checks how many arguments the command line had using a counter and a while loop to iterate through the command line array. Then it checks the last
        argument in the command line array to see if it's an ampersand using the number from the counter. If there was an ampersand, it sets the ampersand to NULL so it doesn't
        cause any problems later on during execution. Returns the flag wasThereAnAmpersand to the main.

I have created and included a makefile that compiles shell for quality of life purposes.

There are many comments included all over builtins.c, redirectionandpiping.c, and shell.c that go into detail about
the purpose of a certain line of lines of code.

help.txt is a manual for my shell that is printed out whenever the help command is entered into the shell.
You have to be in the same directory as the file in order to access it though.

pseudocode.txt contains the pseudocode that I wrote for this project before starting it. The final project is very different than what I wrote in that
pseudocode.

### TESTING
To test if my shell works, I used the following test script with the specified commands below:

      dir
      pwd
      ls | wc
      ls > file
      echo "file"
      wc < file
          
      ls | grep shell
      ls | grep shell | wc (doesn't work on my shell)
      ls | grep shell | sleep 5 & (doesn't work on my shell)
      sleep 5 &
      sleep 2
          
      exit

Everything works on my shell like how it does on the tcsh shell except for command lines with multiple pipes.
I also tested a simple "Hello World!" program that I created to see if it would execute on my shell and it did.
For redirection, I also tested "ls < file1 > file2" to see if it worked and it also worked along with "ls > file > file2"
too see if it threw an error like it should have and it did.
