#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void cd(char* arg){

	if(chdir(arg) == -1) {
		perror("chdir() failed to change to the specified directory");
		return;
	}

	long MAX_PATH = pathconf(".", _PC_PATH_MAX);

	if(MAX_PATH == -1) {
		perror("Failed to determine the pathname length");
		return;
	}

	char* currentDirectory = (char*) malloc(MAX_PATH);

	if(currentDirectory == NULL) {
		perror("Failed to allocate space for obtaining current working directory");
		return;
	}

	if(getcwd(currentDirectory, MAX_PATH) == NULL) {
		perror("Failed to get current working directory");
		return;
	}

	printf("Currently in %s\n", currentDirectory);
	free(currentDirectory);

}

int main(int argc, char** argv){

	if(argc<2){
		printf("Pass the path as an argument\n");
		return 0;
	}
	cd(argv[1]);
	return 0;
}
