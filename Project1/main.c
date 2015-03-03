/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Ninth Edition
 * Copyright John Wiley & Sons - 2013
 */
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
//#include "utilities.h"

#define MAX_LINE		256 /* 256 chars per line, per command */
#define DELIMS                  " \t\r\n"
#define READ_END 0
#define WRITE_END 1

int operator(char* input) {
	return strchr(input, '&') || strchr(input, '<') || strchr(input, '>')
			|| strchr(input, '|');
}

// Adds spaces when it encounter <, >, |, or & for easier parsing.
char* add_space(char* input) {
	size_t len = strlen(input);
	size_t i;
	char* newInput = malloc(MAX_LINE);
	strcpy(newInput,"");
	for (i = 0; i < len; i++) {
		// NOTE: MUST include null character at end when concatenating strings in C
		if (input[i] == '>' || input[i] == '<' || input[i] == '|'
				|| input[i] == '&') {
			char space[] = " \0";
			char inp[2];
			sprintf(inp, "%c", input[i]);
			inp[1] = '\0';

			strcat(newInput, space);
			strcat(newInput, inp);
			strcat(newInput, space);
		} else {
			char inp[2];
			sprintf(inp, "%c", input[i]);
			inp[1] = '\0';
			strcat(newInput, inp);
		}
	}
	return newInput;
}

int run_utilities(char* string[]) {
	// Run as long as set has one argument
		if (string[0] == NULL) {
			puts("Command not recognized.");
			return 0;
		} else if (!strcmp(string[0], "set") && string[1] != NULL) {
			char *path[2];

			// Parse into (PATH|HOME=[environment]) into array and set environment
			path[0] = strtok(string[1], "=");

			if (!strpbrk(path[0], "HOME") || !strpbrk(path[0], "PATH")) {
				puts("Command not recognized");
			} else {
				path[1] = strtok(NULL, "=");
				if (setenv(path[0], path[1], 1))
					puts("Command not recognized");
			}

			return 0;
		} else if (strpbrk(string[0], "cd") != NULL) {
			if (chdir(string[1]))
				puts("Command not recognized");
			return 0;
		} else if (!strcmp(string[0], "jobs")) {
			// TO DO
			return 0;
		}
	return 1;
}
int main(int argc, char **argv, char **envp) {
	char args[MAX_LINE / 2 + 1]; /* command line (of 80) has max of 40 arguments */
	char* string[MAX_LINE];
	char* filename[MAX_LINE];

	pid_t child_id;
	char *cmd;
	int i, should_fork, background_process, number_pipes, tofile, fromfile,
			command;
	while (1) {
		should_fork = 1;
		background_process = 0;
		number_pipes = 0;
		tofile = 0;
		fromfile = 0;
		command = 1;
		printf("quash> ");
		fflush(stdout);
		if (!fgets(args, MAX_LINE, stdin))
			break;
		if (args[strlen(args) - 2] == '&')
			background_process = 1;

		if (strchr(args, '<'))
			fromfile = 1;
		cmd = add_space(args);
		cmd = strtok(cmd, DELIMS);
		i = 0;
		if (!strcmp(cmd, "exit") || !strcmp(cmd, "quit"))
			return 0;

		i = 0;
		while (filename[i] != NULL) {
			filename[i] = NULL;
			i++;
		}
		i = 0;

		// Tokenize the input string
		while (cmd != NULL) {
			// Command is set to 0 after operator is found because the following strings are
			// either file source/destination or piping
			if (operator(cmd) || !command) {
				if (!command && !strchr(cmd, '>')) {
					filename[tofile] = cmd;
					tofile++;
				}
				command = 0;
			} else if (!strchr(cmd, '&') && command) {
				string[i] = cmd;
				i++;
			} //else if()

			cmd = strtok(NULL, DELIMS);

		}

		string[i] = NULL;

		should_fork = run_utilities(string);

		// Don't fork if running built in function
		// Loop if there are multiple destination files (i.e. ls > a.txt > b.txt)
		if (should_fork) {
			i = 0;
			do {
				pid_t pid = fork();
				if (pid == 0) {
					child_id = getpid();
					// Write output to file if '>' is entered
					if (tofile > 0) {
						int file = open(filename[i], O_CREAT | O_RDWR,
						S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
						dup2(file, STDOUT_FILENO);
						close(file);
					}
					execvp(string[0], string);
					printf("Unknown command\n");
					exit(0);
				} else {
					if (!background_process)
						wait(NULL);
				}
				i++;
			} while (i < tofile);

		}

	}

	return 0;
}
