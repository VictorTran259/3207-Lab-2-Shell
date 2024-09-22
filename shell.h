void execute(char **commandLine, int wasThereAnAmpersand);
int checkForPipes(char **commandLine, int wasThereAnAmpersand);
int scandirectory(char *pathName, char **commandLine);
int checkIfBuiltIn(char **commandLine);
int checkForRedirection(char **commandLine, int wasThereAnAmpersand);
int checkForAmpersand(char **commandLine);