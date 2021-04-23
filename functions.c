/*
* Name: Eric Riemer
* Assignment: 3
* School: Oregon State University
* Course: Operating Systems (CS 344)
* Date: 2/4/2021
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include "functions.h"
#define MAX_LENGTH 2048
#define MAX_ARGS 512

/*
* Get User Input
* Purpose: to get user input, split the user input into arguments,
* get user number of arguments from the user,
* store the list of arguemnts in an array of strings
*/

int getUserInput(char* input, char** arguments, char* inputFile, char* outputFile, bool* bgProcess)
{
    //holds the number of arguments
    int arg_counter = 0;
    //bool input file
    bool inFile = false;
    //bool output file
    bool outFile = false;
    //prompt the user for input
    printf("%s", ": ");
    //flush out the output buffers after printing
    fflush(stdout);
    //get the input from the user
    fgets(input, MAX_LENGTH, stdin);

    //expand any instance of "$$" in a command into the process ID of the smallsh itself
    //duplicate the input string and store it in temp
    char temp[MAX_LENGTH];
    strcpy(temp, input);
    //string to hold temp data
    char temp2[MAX_LENGTH];
    //iterate through each character in the input string, stopping 1 space before the end of the string
    for (int i = 0; i < (strlen(temp) - 1); i++)
    {
        //check if an instance of "$$" was entered by the user
        if (temp[i] == '$' && temp[i + 1] == '$')
        {
            //convert the "$$" into "%d" to be used with the sprintf() function
            temp[i] = '%';
            temp[i + 1] = 'd';
            //replace each instance of "%d" with the pid of the parent process
            strcpy(temp2, temp);
            sprintf(temp, temp2, getpid());
        }

    }
    //if the user entered '&' at the end of the input, set the bgProcess bool to true
    if (temp[strlen(temp) - 2] == '&')
    {
        //prevents the '&' from being inputed as an argument
        temp[strlen(temp) - 2] = '\n';
        //set the background process flag to true
        *bgProcess = true;
    }
    //get each argument from the user input and store it in an array of strings
    char* token = strtok(temp, " \n");
    //iterate through each argument in the list and store them in the array of argument strings
    while (token != NULL)
    {
        //if an output file command was encountered, store the file name in outputFile
        if (outFile)
        {
            strcpy(outputFile, token);
            //skip the output file and don't store it as an argument
            token = strtok(NULL, " \n");
            outFile = false;
            continue;
        }
        //if an input file command was encountered, store the file name in inputFile
        if (inFile)
        {
            strcpy(inputFile, token);
            //skip the input file and don't store it as an argument
            token = strtok(NULL, " \n");
            inFile = false;
            continue;
        }
        //if the token is ">" set the outFile bool to true
        if (strcmp(token, ">") == 0)
        {
            //set the flag to store the output file name on the next loop
            outFile = true;
            //skip the ">" and don't store it as an argument
            token = strtok(NULL, " \n");
            continue;
        }
        //if the token is "<" set the inFile bool to true
        else if (strcmp(token, "<") == 0)
        {
            //set the flag to store the input file name on the next loop
            inFile = true;
            //skip the "<" and don't store it as an argument
            token = strtok(NULL, " \n");
            continue;
        }
        //store the argument in the array of argument strings
        arguments[arg_counter] = strdup(token);
        //move to the next argument
        token = strtok(NULL, " \n");
        //increment the argument counter
        arg_counter++;
    }
    //set the last element in the array to NULL
    arguments[arg_counter] = NULL;
    //return the number of arguments
    return arg_counter;
}

/*
* This function free's the memory of the array of arguments
*/

void freeMemory(int number_of_arguments, char** arguments)
{
    for (int i = 0; i < number_of_arguments; i++) {
        free(arguments[i]);
    }
}

/*
* Built in cd command
* Purpose: changes the working directory of smallsh
* With no arguments - it changes to the directory specified in the
* HOME environment variable
* This command can also take one argument: the path of a directory to change to.
* Your cd command should support both absolute and relative paths.
*/

void cd(int number_of_arguments, char** arguments)
{
    //if there is only the "cd" command without arguments, change the directory to "HOME" directory
    if (number_of_arguments == 1)
    {
        chdir(getenv("HOME"));
    }
    //else, change the directory to the user specified location
    else
    {
        chdir(arguments[1]);
    }
}

/*
* Built in status command
* Purpose: prints out either the exit status or the terminating signal
* of the last foreground process ran by your shell.
* If this command is run before any foreground command is run,
* then it should simply return the exit status 0.
* The three built-in shell commands do not count as
* foreground processes for the purposes of this built-in command
* - i.e., status should ignore built-in commands.
* Reference: "https://man7.org/linux/man-pages/man2/wait.2.html"
*/

void printStatus(int* status)
{
    //if the child process was terminated normally
    if (WIFEXITED(*status))
    {
        //return the exit status value the child
        printf("exit value %d\n", WEXITSTATUS(*status));
        fflush(stdout);
    }
    //else if, the child process was terminated abnormally
    else if (WIFSIGNALED(*status))
    {
        //return the signal number that caused the child to terminate
        printf("terminated by signal %d\n", WTERMSIG(*status));
        fflush(stdout);
    }
}

//Signal handling funtion for SIGTSTP not in foreground mode
void SIGTSTP_handler(int signal)
{
    if (!fgMode)
    {
        char* output = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, output, strlen(output));
        fgMode = true;
    }

    else
    {
        char* output = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, output, strlen(output));
        fgMode = false;
    }

    write(STDOUT_FILENO, ": ", 2);
       
}


/*
* Choose Command
* Based on the user input, pick a command to run
*/
void chooseCommand(int number_of_arguments, char** arguments, char* inputFile, char* outputFile, bool bgProcess, int* status)
{
    //file descriptor
    int fd;
    //holds the error code
    int error;
    //holds the pid of the child process
    pid_t childPid;
    //if the first argument is a newline or a '#' character, do nothing
    if (arguments[0] == NULL || strstr(arguments[0], "#") != NULL)
    {
        //do nothing
    }
    //else if, the user enters the command "exit", exit th program
    else if (strcmp(arguments[0], "exit") == 0)
    {
        //free the memory in the arguments array
        freeMemory(number_of_arguments, arguments);
        //call the pre-built exit() function
        exit(0);
    }
    //else if the user enters the command "cd", change the directory
    else if (strcmp(arguments[0], "cd") == 0)
    {
        //call the pre-built cd() function
        cd(number_of_arguments, arguments);
    }
    //else if the user enters the command "status", print out either the exit status or the terminating signal
    //of the last foreground process ran by the shell
    else if (strcmp(arguments[0], "status") == 0)
    {
        //call the pre-built status() function
        printStatus(status);
    }

    //execute other commands
    else
    {
        //create a child process by calling fork()
        childPid = fork();
        //if the fork() failed, prompt the user and exit the program
        if (childPid == -1)
        {
            perror("fork() failed!");
            freeMemory(number_of_arguments, arguments);
            exit(1);
        }
        //rung the child process
        else if (childPid == 0)
        {
            
            //child running as a foreground process will terminate itself when it receives SIGINT
            if (!bgProcess)
            {
                //set the SIGINT action back to default from ignore
                struct sigaction SIGINT_action_fg = { 0 };
                SIGINT_action_fg.sa_handler = SIG_DFL;
                sigaction(SIGINT, &SIGINT_action_fg, NULL);
            }
            //else if child is running in the background, and in foreground mode, set SIGINT action back to default from ignore
            else if (fgMode)
            {
                //set the SIGINT action back to default
                struct sigaction SIGINT_action_fg = { 0 };
                SIGINT_action_fg.sa_handler = SIG_DFL;
                sigaction(SIGINT, &SIGINT_action_fg, NULL);
            }
            //input file redirected via stdin is opened for reading only; 
            //if the file cannot be opened for reading, 
            //it prints an error message and sets the exit status to 1
            if (inputFile[0] != '\0')
            {
                fd = open(inputFile, O_RDONLY);
                //print an error if unable to open the input file for reading
                if (fd == -1)
                {
                    fprintf(stderr, "Unable to open input file: %s\n", inputFile);
                    fflush(stdout);
                    freeMemory(number_of_arguments, arguments);
                    exit(1);
                }
                else 
                {
                    //redirect stdin to read from the input file
                    error = dup2(fd, 0);
                    if (error == -1)
                    {
                        fprintf(stderr, "error redirecting the input file");
                        freeMemory(number_of_arguments, arguments);
                        exit(1);
                    }  
                }
                close(fd);
            }
            //if the user doesn't redirect the standard input for a background command, then standard input is redirected to /dev/null
            else if (bgProcess)
            {
                fd = open("/dev/null", O_RDONLY);
                //print an error if unable to open the input file for reading
                if (fd == -1)
                {
                    fprintf(stderr, "Unable to open file\n");
                    fflush(stdout);
                    freeMemory(number_of_arguments, arguments);
                    exit(1);
                }
                else
                {
                    //redirect stdin to read from the input file
                    error = dup2(fd, 0);
                    if (error == -1)
                    {
                        fprintf(stderr, "error redirecting the input file");
                        freeMemory(number_of_arguments, arguments);
                        exit(1);
                    }
                }
                close(fd);
            }
            //output file redirected via stdout is opened for writing only; 
            //if the file cannot be opened for writing, 
            //it prints an error message and sets the exit status to 1
            if (outputFile[0] != '\0')
            {
                fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                //print an error if unable to open the output file for writing
                if (fd == -1)
                {
                    fprintf(stderr, "Unable to open output file: %s\n", outputFile);
                    fflush(stdout);
                    freeMemory(number_of_arguments, arguments);
                    exit(1);
                }
                else
                {
                    //redirect stdout to write to the output file
                    error = dup2(fd, 1);
                    if (error == -1)
                    {
                        fprintf(stderr, "error redirecting the output file");
                        freeMemory(number_of_arguments, arguments);
                        exit(1);
                    }
                }
                close(fd);
            }
            //If the user doesn't redirect the standard output for a background command, then standard output is redirected to /dev/null
            else if (bgProcess)
            {
                fd = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                //print an error if unable to open the output file for writing
                if (fd == -1)
                {
                    fprintf(stderr, "Unable to open the file\n");
                    fflush(stdout);
                    freeMemory(number_of_arguments, arguments);
                    exit(1);
                }
                else
                {
                    //redirect stdout to write to the output file
                    error = dup2(fd, 1);
                    if (error == -1)
                    {
                        fprintf(stderr, "error redirecting the output file");
                        freeMemory(number_of_arguments, arguments);
                        exit(1);
                    }
                }
                close(fd);
            }

            //execute the command passed by the user
            error = execvp(arguments[0], arguments);
            if (error == -1)
            {
                //if the user entered an incorrect command, print the error
                fprintf(stderr, "Unable to execute the command\n");
                fflush(stdout);
                freeMemory(number_of_arguments, arguments);
                exit(1);
            }
            
        }
        //parent process
        else 
        {
            //if the child process is running in the background, and foreground only mode is off, print its pid and don't wait
            if (bgProcess == true && fgMode == false)
            {
                waitpid(childPid, status, WNOHANG);
                printf("Background pid is %d\n", childPid);
                fflush(stdout);
  
            }
            //if the child proces is to run in the foreground, wait for the child process to finish
            else
            {
                waitpid(childPid, status, 0);
                //if child is terminated abnormally, print the signal that terminated it
                if (WIFSIGNALED(*status))
                {
                    //return the signal number that caused the child to terminate
                    printf("terminated by signal %d\n", WTERMSIG(*status));
                    fflush(stdout);
                }
            }
        }
    }
    //if a child finishes while running in the background, print the message that it has finished
    while ((childPid = waitpid(-1, status, WNOHANG)) > 0)
    {
        printf("background pid %d is done: ", childPid);
        fflush(stdout);
        printStatus(status);
    }
    //set the input file to empty
    inputFile[0] = '\0';
    //set the output file to empty
    outputFile[0] = '\0';
    //free the memory in the arguments array
    freeMemory(number_of_arguments, arguments);
}