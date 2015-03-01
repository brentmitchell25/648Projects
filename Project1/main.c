/**
 * Simple shell interface program.
 *
 * Operating System Concepts - Ninth Edition
 * Copyright John Wiley & Sons - 2013
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define MAX_LINE		80 /* 80 chars per line, per command */
#define DELIMS                  " \t\r\n"
int main(int argc, char **argv, char **envp) {
	char args[MAX_LINE / 2 + 1]; /* command line (of 80) has max of 40 arguments */
	char* string[MAX_LINE];

	pid_t child_id;
	int should_run = 1;
	char *cmd;
	int i, upper, should_fork;
	while (should_run) {
		should_fork = 1;
		printf("quash> ");
		fflush(stdout);
		if (!fgets(args, MAX_LINE, stdin))
			break;
		cmd = strtok(args, DELIMS);
		int i = 0;
		if (!strcmp(cmd, "exit") || !strcmp(cmd, "quit"))
			return 0;

		while (cmd != NULL) {
			string[i] = cmd;
			cmd = strtok(NULL, DELIMS);
			i++;
		}

		if (strpbrk(string[0], "cd") != NULL) {
			chdir(string[1]);
			puts("HERE");
			should_fork = 0;
		}

		string[i] = NULL;

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

		/**
		 * After reading user input, the steps are:
		 * (1) fork a child process
		 * (2) the child process will invoke execvp()
		 * (3) if command included &, parent will invoke wait()
		 */
	}

	return 0;
}
