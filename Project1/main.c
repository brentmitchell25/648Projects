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
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include "jobs.h"

#define MAX_LINE		256 /* 256 chars per line, per command */
#define DELIMS                  " \t\r\n"
#define READ_END 0
#define WRITE_END 1

char* dir;
struct Main m;
struct Pipes {
	int fd[2];
};
int operator(char* input) {
	return strchr(input, '&') || strchr(input, '<') || strchr(input, '>')
			|| strchr(input, '|');
}

// SIGCHLD signal handler to reap zombie processes.
void sig_child_handler(int signal) {
	pid_t pid;
	while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
		delete_job(&m, pid);

}

// Set up the above SIGCHLD handler so it will go into action.
// This should be called at Quash startup.
void init_zombie() {
	struct sigaction sa;

	sa.sa_handler = sig_child_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction( SIGCHLD, &sa, NULL) == -1) {
		printf("Failure in sigaction() call.");
		exit(1);
	}
}

// Adds spaces when it encounter <, >, |, or & for easier parsing.
char* add_space(char* input) {
	size_t len = strlen(input);
	size_t i;
	char* newInput = malloc(MAX_LINE);
	strcpy(newInput, "");
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
	} else if (!strcmp(string[0], "exit") || !strcmp(string[0], "quit"))
		exit(0);
	else if (!strcmp(string[0], "set") && string[1] != NULL) {
		char *path[2];
		// Parse into (PATH|HOME=[environment]) into array and set environment
		path[0] = strtok(string[1], "=");

		if (!strpbrk(path[0], "HOME") && !strpbrk(path[0], "PATH")) {
			puts("Path not found");
		} else {
			// Sets path entered by user
			// If user enters no path, then clear it.
			path[1] = strtok(NULL, "=");

			if (path[1] != '\0' && setenv(path[0], path[1], 1))
				puts("Path not found");
			else if (path[1] == '\0' && setenv(path[0], "", 1)) {
				puts("Path not found");
			}
		}

		return 0;
	} else if (strcmp(string[0], "cd") == 0) {
		if (chdir(string[1])){
			if (string[1] == '\0'  && !chdir(getenv("HOME")));
			else if(!strcmp(string[1],"~") && !chdir(getenv("HOME")));
			else
				puts("Directory not found.");
		}
		return 0;
	} else if (!strcmp(string[0], "jobs")) {
		print_jobs(&m);
		return 0;
	} else if (!strcmp(string[0], "kill")) {
		printf("%s",string[1]);
		if(string[1] == NULL)
			puts("Need to specify a PID to kill.");
		else {
			if(kill(atoi(string[1]),SIGKILL))
				printf("Error trying to kill PID: %d",errno);
		}
		return 0;
	}

	return 1;
}

int main(int argc, char **argv, char **envp) {
	char args[MAX_LINE / 2 + 1]; /* command line (of 80) has max of 40 arguments */

	char* to_filename[MAX_LINE];
	char* from_filename[MAX_LINE];

	init_zombie();
	char *cmd;
	int i, should_fork, should_pipe, background_process, redirect_output,
			redirect_input, pipe_number, tofile, fromfile, command;
	int jobs = 0;
	int status = 0;
	TAILQ_INIT(&m.head);
	strcpy(m.main, "Main"); 
	m.pid = getpid();

	while (1) {
		should_fork = 1;
		should_pipe = 0;
		command = 1;
		background_process = 0;
		redirect_output = 0;
		redirect_input = 0;
		pipe_number = -1;
		tofile = 0;
		fromfile = 0;
		int pipe_count = -1;
		int pipes[MAX_LINE][2];

		dir = getcwd(dir, 1024);
		char quash[1024];
		sprintf(quash, "[quash:%s]$ ", dir);
		printf("%s", quash);
		fflush(stdout);
		if (!fgets(args, MAX_LINE, stdin))
			break;

		cmd = add_space(args);
		cmd = strtok(cmd, DELIMS);


		for (i = 0; args[i]; i++) {
			pipe_count += (args[i] == '|');
		}

		if (pipe_count > -1)
			pipe_count++;

		to_filename[0] = NULL;
		from_filename[0] = NULL;

			i = 0;

		do {
			i = 0;
			char* string[MAX_LINE];
			string[0] = NULL;
			if (pipe_count > -1 && pipe_number >= pipe_count - 1) {
				pipe_number++;

				should_pipe = 0;
			}
			// Tokenize the input string
			while (cmd != NULL) {
				// Command is set to 0 after operator is found because the following strings are
				// either file source/destination or piping
				if (operator(cmd) || !command) {
					if (strchr(cmd, '&')) {
						background_process = 1;
						command = 0;
					} else if (strchr(cmd, '>')) {
						redirect_output = 1;
						command = 0;
					} else if (strchr(cmd, '<')) {
						redirect_input = 1;
						command = 0;
					} else if (strchr(cmd, '|')) {
						pipe_number++;
						if (pipe(pipes[pipe_number]) < 0)
							perror("Failed to open pipe");

						command = 1;
						should_pipe = 1;
						cmd = strtok(NULL, DELIMS);
						break;
					} else if (!command && redirect_output) {
						to_filename[tofile] = cmd;
						tofile++;
						command = 0;
					} else if (!command && redirect_input) {
						from_filename[fromfile] = cmd;
						fromfile++;
						command = 0;
					}

				} else if (!strchr(cmd, '&') && command) {
					string[i] = cmd;
					i++;
				} //else if()

				cmd = strtok(NULL, DELIMS);

			}

			string[i] = '\0';
			to_filename[tofile+1] = NULL;
			from_filename[fromfile+1] = NULL;
			should_fork = run_utilities(string);
			// Don't fork if running built in function
			// Loop if there are multiple destination files (i.e. ls > a.txt > b.txt)
			if (should_fork) {
				i = 0;
				do {
					jobs++;
					pid_t pid = fork();
					if (pid == 0) {
						if (background_process) {
							setpgid(0, 0);
						}
						if (pipe_number > -1) {
							if (pipe_number == 0) {
								if (dup2(pipes[pipe_number][WRITE_END],
								STDOUT_FILENO) < 0) {
									perror("dup2");
									exit(EXIT_FAILURE);
								}
								//close(pipes[pipe_number][WRITE_END]);
								//close(pipes[pipe_number][READ_END]);

							} else if (pipe_number == pipe_count) {
								if (dup2(pipes[pipe_number - 1][READ_END],
								STDIN_FILENO) < 0) {
									perror("dup2");
									exit(EXIT_FAILURE);
								}
								//close(pipes[pipe_number-1][READ_END]);
								//close(pipes[pipe_number-1][WRITE_END]);
							} else {
								dup2(pipes[pipe_number - 1][READ_END],
								STDIN_FILENO);
								dup2(pipes[pipe_number][WRITE_END],
								STDOUT_FILENO);
							}
							int j = 0;

							 for(j= 0 ; j < pipe_count; j++) {

							 if(close(pipes[j][READ_END]) < 0) {
							 perror("Read Close");
							 exit(EXIT_FAILURE);
							 }
							 }

						}
						// Read input file if '<' is entered
						if (i < fromfile && fromfile > 0) {
							int file_in = open(from_filename[i], O_RDWR,
							S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
							printf("%d", file_in);
							dup2(file_in, STDIN_FILENO);
							close(file_in);
						}
						// Write output to file if '>' is entered
						if (i < tofile && tofile > 0) {
							int file_out = open(to_filename[i],
							O_CREAT | O_RDWR,
							S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
							dup2(file_out, STDOUT_FILENO);
							close(file_out);
						}

						if (execvp(string[0], string) < 0) {
							perror("Execute:");

						}
						printf("Unknown command\n");
						exit(0);
					} else {
						int j = 0;

						if (!background_process) {
							waitpid(pid, &status, 0);
						}

						else {
							add_job(&m, *string, jobs, pid);
							printf("[%d]\t%d\t%s\n", jobs, pid, *string);
							sleep(1);
						}
						if (should_pipe) {
							close(pipes[pipe_number][WRITE_END]);
						}
					}
					i++;
				} while (i < tofile || i < fromfile);
			}

			// Need to deallocate memory from add_space function
			//free(cmd);
		} while (pipe_number < pipe_count && should_pipe);
	}

	return 0;
}
