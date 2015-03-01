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
	int i, should_fork;
	while (1) {
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

		puts(string[0]);

		if(!strcmp(string[0],"set")) {
			should_fork = 0;
		} else if (strpbrk(string[0], "cd") != NULL) {
			if(chdir(string[1]))
				puts("Command not recognized");
			should_fork = 0;
		} else if(strpbrk(string[0],"jobs") == NULL) {
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

	}

	return 0;
}
