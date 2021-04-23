/*
* Name: Eric Riemer
* Assignment: 3
* School: Oregon State University
* Course: Operating Systems (CS 344)
* Date: 2/4/2021
*
*/
#ifndef FUNCTIONS_H
#define FUNCTIONS_H
	bool fgMode;
	int getUserInput(char* input, char** arguments, char* inputFile, char* outputFile, bool* bgProcess);
	void freeMemory(int number_of_arguments, char** arguments);
	void cd(int number_of_arguments, char** arguments);
	void printStatus(int* status);
	void SIGTSTP_handler(int signal);
	void chooseCommand(int number_of_arguments, char** arguments, char* inputFile, char* outputFile, bool bgProcess, int* statusCode);
#endif