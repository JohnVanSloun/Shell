#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>

#include "util.h"

#define WRITE (O_WRONLY | O_CREAT)
#define PERM (S_IRUSR | S_IWUSR)

// helper function to print [4061-shell] and the current working directory
void shellPrint();

// Given an array of tokens, the number of tokens, the specified index to begin looking for commands, the function will return an array of commands
// containing the commmands for that command, stopping when it reaches either a pipe or output redirection command. The function will modify int* 
// NUM_ARGS to the number of commands in the returned character array. Note that the number of commands also includes a NULL pointer command at 
// the last index of the returned array.
char** loadCommands(char** tokens, int NUM_TOKENS, int index, int* NUM_ARGS);

#define MAX_INPUT_SIZE 100 // maximum amount of characters allowed for a given line of input passed onto the shell

int main(){
	// Save starting directory
	long MAX_PATH = pathconf(".", _PC_PATH_MAX);
	if(MAX_PATH == -1) {
		perror("Failed to determine the pathname length");
		return -1;
	}
	char* startDirectory = (char*) malloc(MAX_PATH);
	if(startDirectory == NULL) {
		perror("Failed to allocate space for obtaining current working directory");
		return -1;
	}
	if(getcwd(startDirectory, MAX_PATH) == NULL) {
		perror("Failed to get current working directory");
		return -1;
	}

	char* currDirectory = (char*) malloc(MAX_PATH);
	if(currDirectory == NULL) {
		perror("Failed to allocate space for obtaining current working directory");
		return -1;
	}

	while(1) {
		char input[MAX_INPUT_SIZE] = ""; // store command line input
		shellPrint();
		if( read(STDIN_FILENO, input, sizeof(input)) == -1) { // read command line input
			perror("Shell had an error reading the next input of the command line");
			return -1;
		}

		char* tokens[50];
		int NUM_TOKENS = parse_line(input, tokens, " \n");
		if ( get_command_type(tokens[0]) == EXIT) {
			printf("Exiting shell\n");
			free(startDirectory);
			free(currDirectory);
			return 0;
		}

		int NUM_ARGS_FIRST_CMD;
		char** first_cmd_args = loadCommands(tokens, NUM_TOKENS, 0, &NUM_ARGS_FIRST_CMD);
		
		if( ( NUM_ARGS_FIRST_CMD - 1) == NUM_TOKENS) { // only one command and no output redirection/pipes

			pid_t pid = fork();

			if(pid < 0) {
				perror("fork() failed");
			}
			else if(pid == 0) {
				if(get_command_type(tokens[0]) == 4) { // Not one of the executable files
					execvp(tokens[0], first_cmd_args);
				}
				// Use created commands
				else {
					strcpy(currDirectory, startDirectory);
					execvp(strcat(strcat(currDirectory, "/"), tokens[0]), first_cmd_args);
				}
				perror("Command error");
				exit(EXIT_FAILURE); 
			}
			else {
				if(wait(NULL) == -1) {
					perror("failed to wait for child process to terminate");
				}
				// Change directory of the program if cd is used
				if(get_command_type(tokens[0]) == CD){
					if(chdir(first_cmd_args[1]) == -1) {
						perror("chdir() failed to change to the specified directory");
					}
				}
			}
		}
		else { // either output redirection, and/or piping
			int NUM_ARGS_SECOND_CMD;
			char** second_cmd_args = loadCommands(tokens, NUM_TOKENS, NUM_ARGS_FIRST_CMD, &NUM_ARGS_SECOND_CMD);
			int NUM_ARGS_THIRD_CMD;
			char** third_cmd_args = loadCommands(tokens, NUM_TOKENS, NUM_ARGS_FIRST_CMD + NUM_ARGS_SECOND_CMD, &NUM_ARGS_THIRD_CMD);
			int fd;
			int pfd[2];
			pid_t pid;

			// Check for redirection
			if(strcmp(tokens[NUM_ARGS_FIRST_CMD - 1], ">") == 0 || strcmp(tokens[NUM_ARGS_FIRST_CMD - 1], ">>") == 0) { // Check if the first command is being redirected (>)
				pid = fork();

				if(pid < 0) { // Forking fail
					perror("fork() failed");
				} else if(pid == 0) { // Child process
					if(strcmp(tokens[NUM_ARGS_FIRST_CMD - 1], ">") == 0) { // Truncating redirection
						fd = open(tokens[NUM_ARGS_FIRST_CMD], WRITE | O_TRUNC, PERM);
					} else { // Appending redirection
						fd = open(tokens[NUM_ARGS_FIRST_CMD], WRITE | O_APPEND, PERM);
					}

					if(fd < 0) { // Check file descriptor for error
						perror("Error when opening file");
						exit(EXIT_FAILURE);
					}

					if(dup2(fd, STDOUT_FILENO) < 0) { // Duplicate the open file onto the standard output
						perror("Error when copying file descriptor to STDOUT");
					}

					close(fd);

					if(get_command_type(tokens[0]) == 4) { // Not one of the executable files
						execvp(tokens[0], first_cmd_args);
					} else { // One of the executable files
						strcpy(currDirectory, startDirectory);
						execvp(strcat(strcat(currDirectory, "/"), tokens[0]), first_cmd_args);
					}

					perror("Command error");
					exit(EXIT_FAILURE);

				} else {
					if(wait(NULL) == -1) { // Wait for child process to finish or catch failure to wait
						perror("failed to wait for child process to terminate");
					}
				}
			} else if(strcmp(tokens[NUM_ARGS_FIRST_CMD - 1], "|") == 0) {

				// Open pipe and check for error
				if(pipe(pfd) == -1) {
					perror("Error opening pipe");
					exit(EXIT_FAILURE);
				}
				
				pid = fork();

				if(pid==0) {

					close(pfd[0]); // Close read end
					// Copy write file drescriptor to STDOUT
					if(dup2(pfd[1], STDOUT_FILENO) < 0) {
						perror("Error when copying file descriptor to STDOUT");
					}

					close(pfd[1]); // Close write end

					if(get_command_type(tokens[0]) == 4) { // Not one of the executable files
						execvp(tokens[0], first_cmd_args);
					} else { // One of the executable files
						strcpy(currDirectory, startDirectory);
						execvp(strcat(strcat(currDirectory, "/"), tokens[0]), first_cmd_args);
					}

					perror("Command error");
					exit(EXIT_FAILURE);
				} else {

					if(wait(NULL) == -1) { // wait for child process to finish writing
						perror("failed to wait for child process to terminate");
						exit(EXIT_FAILURE);
					}

					pid = fork(); // fork another child process to execute the second command

					if(pid == 0) {
						// Copy read file descriptor to STDIN
						close(pfd[1]); // Close write end
						if(dup2(pfd[0], STDIN_FILENO) < 0) {
							perror("Error when copying file descriptor to STDIN");
						}
						close(pfd[0]); // Close read end

						if (third_cmd_args[0] != NULL) {
							if(strcmp(tokens[NUM_ARGS_FIRST_CMD + NUM_ARGS_SECOND_CMD - 1], ">") == 0 || strcmp(tokens[NUM_ARGS_FIRST_CMD + NUM_ARGS_SECOND_CMD- 1], ">>") == 0) {
								if(strcmp(tokens[NUM_ARGS_FIRST_CMD + NUM_ARGS_SECOND_CMD - 1], ">") == 0) { // Truncating redirection
									fd = open(tokens[NUM_ARGS_FIRST_CMD + NUM_ARGS_SECOND_CMD], WRITE | O_TRUNC, PERM);
								} else { // Appending redirection
									fd = open(tokens[NUM_ARGS_FIRST_CMD + NUM_ARGS_SECOND_CMD], WRITE | O_APPEND, PERM);
								}

								if(fd < 0) { // Check file descriptor for error
									perror("Error when opening file");
									exit(EXIT_FAILURE);
								}

								if(dup2(fd, STDOUT_FILENO) < 0) { // Duplicate the open file onto the standard output
									perror("Error when copying file descriptor to STDOUT");
								}

								close(fd);

								if(get_command_type(tokens[NUM_ARGS_FIRST_CMD + NUM_ARGS_SECOND_CMD]) == 4) { // Not one of the executable files
									execvp(tokens[NUM_ARGS_FIRST_CMD], second_cmd_args);
								} else { // One of the executable files
									strcpy(currDirectory, startDirectory);
									execvp(strcat(strcat(currDirectory, "/"), tokens[NUM_ARGS_FIRST_CMD]), second_cmd_args);
								}

								perror("Command error");
								exit(EXIT_FAILURE);
							}
						}
						
						if(get_command_type(tokens[NUM_ARGS_FIRST_CMD]) == 4) { // Not one of the executable files
							execvp(tokens[NUM_ARGS_FIRST_CMD], second_cmd_args);
						} else { // One of the executable files
							strcpy(currDirectory, startDirectory);
							execvp(strcat(strcat(currDirectory, "/"), tokens[NUM_ARGS_FIRST_CMD]), second_cmd_args);
						} 

						perror("Command error");
						exit(EXIT_FAILURE);
					} else {
						// Closes up both ends of pipe
						close(pfd[0]);
						close(pfd[1]);

						if(wait(NULL) == -1) { // wait for child process to finish executing
							perror("failed to wait for child process to terminate");
							exit(EXIT_FAILURE);
						}
					}
				}
			}
			free(third_cmd_args);
			free(second_cmd_args);
		}

		free(first_cmd_args);
	}
}

// helper function to print [4061-shell] and the current working directory
void shellPrint() {

	long MAX_PATH = pathconf(".", _PC_PATH_MAX);

	if(MAX_PATH == -1) {
		perror("Failed to determine the pathname length");
		exit(EXIT_FAILURE);
	}

	char* currentDirectory = (char*) malloc(MAX_PATH);

	if(currentDirectory == NULL) {
		perror("Failed to allocate space for obtaining current working directory");
		exit(EXIT_FAILURE);
	}

	if(getcwd(currentDirectory, MAX_PATH) == NULL) {
		perror("Failed to get current working directory");
		exit(EXIT_FAILURE);
	}

	printf("[4061-shell]%s $ ", currentDirectory);
	fflush(stdout);
	free(currentDirectory);

}

// Given an array of tokens, the number of tokens, the specified index to begin looking for commands, the function will return an array of commands
// containing the commmands for that command, stopping when it reaches either a pipe or output redirection command. The function will modify int* 
// NUM_ARGS to the number of commands in the returned character array. Note that the number of commands also includes a NULL pointer command at 
// the last index of the returned array.
char** loadCommands(char** tokens, int NUM_TOKENS, int index, int* NUM_ARGS) {
	bool exitLoop = false;
	int tempIndex = index;
	int numberOfArgs = 0;
	while(!exitLoop && ( index < NUM_TOKENS) ) {
		if( ( strcmp(tokens[index], "|") == 0 ) || ( strcmp(tokens[index], ">") == 0 ) || ( strcmp(tokens[index], ">>")) == 0	) { // stop when a pipe or output redirection is detected
			exitLoop = true;
		}
		else {
			numberOfArgs++;
		}
		index++;
	}
	numberOfArgs++; // Add one argument for null pointer as last index of array

	index = tempIndex;

	*NUM_ARGS = numberOfArgs;

	char** args = (char** ) malloc(numberOfArgs * sizeof(char* ));
	if(args == NULL) {
		perror("malloc() failed in loadCommands() function");
		exit(EXIT_FAILURE);
	}

	for(int i = 0; i < numberOfArgs-1; i++) {
		args[i] = tokens[i + index]; // Each element of args will point to an element in token; do not need to allocate memory for this
	}
	args[numberOfArgs-1] = NULL;

	return args;

}
