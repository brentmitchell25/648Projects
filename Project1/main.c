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
#include <string.h>
#include <sys/types.h>

#define MAX_LINE		80 /* 80 chars per line, per command */
#define DELIMS                  " \t\r\n"
#define READ_END 0
#define WRITE_END 1

int main(int argc, char **argv, char **envp) {
	char args[MAX_LINE / 2 + 1]; /* command line (of 80) has max of 40 arguments */
	char* string[MAX_LINE];

	pid_t child_id;
	char *cmd;
	int i, should_fork, background_process, number_pipes;
	while (1) {
		should_fork = 1;
		background_process = 0;
		number_pipes = 0;
		printf("quash> ");
		fflush(stdout);
		if (!fgets(args, MAX_LINE, stdin))
			break;
		cmd = strtok(args, DELIMS);
		i = 0;
		if (!strcmp(cmd, "exit") || !strcmp(cmd, "quit"))
			return 0;

		while (cmd != NULL) {
			string[i] = cmd;
			cmd = strtok(NULL, DELIMS);
			if(strchr(string[i],'&'))
				background_process = 1;
			if(strchr(string[i],'|'))
				number_pipes++;
			i++;
		}
		string[i] = NULL;

		// Run as long as set has one argument
		if(!strcmp(string[0],"set") && string[1] != NULL) {
			i = 0;
			char *path[2];

			// Parse into (PATH|HOME=[environment]) into array and set environment
			path[0] = strtok(string[1],"=");

			if(!strpbrk(path[0],"HOME") || !strpbrk(path[0],"PATH") ) {
				puts("Unknown Command");
			} else {
				path[1] = strtok(NULL,"=");
				if(setenv(path[0],path[1],1))
					puts("Unknown Command");
			}

			should_fork = 0;
		} else if (strpbrk(string[0], "cd") != NULL) {
			if(chdir(string[1]))
				puts("Command not recognized");
			should_fork = 0;
		} else if(strpbrk(string[0],"jobs") == NULL) {
			should_fork = 0;
		}

		// Don't fork if running built in function
		if (should_fork) {
			pid_t pid = fork();
			if (pid == 0) {
				child_id = getpid();
				execvp(string[0], string);
				printf("Unknown command\n");
				exit(0);
			} else {
				wait(NULL);
			}
		}

	}

	return 0;
}
