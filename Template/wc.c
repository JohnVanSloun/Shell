#include<stdlib.h>
#include<stdio.h>
#include <unistd.h>
#include<string.h>

int read_from_stdin = 0;

// Find the line, word, and character count of a file
void wc(int mode, char* path){

	// Error if no file or file path is given
	if (path == NULL && !read_from_stdin){
		printf("Error: no file provided\n");
		return;
	}

	// Counter variables
	int lines = 0;
	int words = 0;
	int characters = 0;

	int is_word = 0;
	char ch;
	FILE *input;
	if(!read_from_stdin) {
		input = fopen(path, "r");
	}
	else {
		input = stdin;
	}

	// Error if file or file path is not found
	if(!input) {
		printf("Error: File %s not found\n", path);
		return;
	}

	// Iterate through each character in file
	while ((ch = fgetc(input)) != EOF){
		// Increase character count
		characters++;
		// If new line, increase line count and end current word
		if (ch == '\n'){
			lines++;
			is_word = 0;
		}
		// If space or tab, end current word
		else if ((ch == ' ') || (ch == '\t')){
			is_word = 0;
		}
		// If not whitespace and no current word,
		// increment word count and start current word
		else if (!is_word){
			words++;
			is_word = 1;
		}
	}

	fclose(input);

	// Print
	switch(mode){
		// Default mode
		case 0:
			printf("	%d	%d	%d\n", lines, words, characters);
			break;
		// -l (lines) mode
		case 1:
			printf("	%d\n", lines);
			break;
		// -w (words) mode
		case 2:
			printf("	%d\n", words);
			break;
		// -c (characters) mode
		case 3:
			printf("	%d\n", characters);
			break;
	}
}

int main(int argc, char** argv){

	if(!isatty(STDIN_FILENO)) { // check if stdin is not in a terminal, which means that wc was executed by shell.c in the form of a pipe where wc will receive output from stdin
		if(argc <= 2) {
			read_from_stdin = 1;
		}
	}

	if(argc>2){ // input file given - ignore piping
		if(strcmp(argv[1], "-l") == 0) {
			wc(1, argv[2]);
		} else if(strcmp(argv[1], "-w") == 0) { 
			wc(2, argv[2]);
		} else if(strcmp(argv[1], "-c") == 0) { 
			wc(3, argv[2]);
		} else {
			printf("Invalid arguments\n");
		}
	} else if (argc==2){ // handles piping case, if no file/piping will pass NULL to path.
	 	if(strcmp(argv[1], "-l") == 0) { 
			wc(1, NULL);
		} else if(strcmp(argv[1], "-w") == 0) { 
			wc(2, NULL);
		} else if(strcmp(argv[1], "-c") == 0) { 
			wc(3, NULL);
		} else {
    		wc(0, argv[1]);
    	}
  	} else {
  		wc(0,NULL);
  	}

	return 0;
}
