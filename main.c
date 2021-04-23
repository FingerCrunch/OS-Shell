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
*       Compile the program as follows:
*       gcc --std=gnu99 -g -o smallsh main.c functions.c
*/

//global bool variable to switch between foreground mode 
bool fgMode = false;

int main()
{
    //string to hold user input in the command line
    char userInput[MAX_LENGTH];
    //an array of strings that holds each argument passed by the user
    char *argument_list[MAX_ARGS];
    //holds the number of arguments passed by the user
    int number_of_arguments;
    //bool that keeps the while loop looping
    bool runShell = true;
    //string to hold the input file
    char inputFile[MAX_LENGTH];
    inputFile[0] = '\0';
    //string to hold the output file
    char outputFile[MAX_LENGTH];
    outputFile[0] = '\0';
    //bool for background process
    bool bgProcess = false;
    //holds the status
    int status = 0;

    //custom handler for SIGTSTP
    //Reference: OSU CS344, Module 5, "Exploration: Signal Handling API"
    struct sigaction SIGTSTP_action = { 0 };
    SIGTSTP_action.sa_handler = SIGTSTP_handler;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    //struct for the SIGINT signal
    struct sigaction SIGINT_action = { 0 };
    
    while (runShell)
    {
        //ignore the SIGINT in the parent process
        //Reference: OSU CS344, Module 5, "Exploration: Signal Handling API"
        SIGINT_action.sa_handler = SIG_IGN;
        sigfillset(&SIGINT_action.sa_mask);
        sigaction(SIGINT, &SIGINT_action, NULL);
        //prompt and get user input
        //split the user input into arguments
            //get user number of arguments from the user
            //store the list of arguemnts in an array of strings
        number_of_arguments = getUserInput(userInput, argument_list, inputFile, outputFile, &bgProcess);
        //determine which command to perform based on the arguments
        //execute the command the user entered
        chooseCommand(number_of_arguments, argument_list, inputFile, outputFile, bgProcess, &status);
        //set the background process to false
        bgProcess = false;
    };

    return 0;
}