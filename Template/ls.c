#include<stdlib.h>
#include<stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// Counts the number of files/subdirectories in a directory
int get_dir_count(char *path) {
	int count = 0;
	struct dirent *entry;
	DIR *dir;
	
	// Checks that path is not null
	if (path == NULL) {
		perror("Could not access current directory.");
	}
	
	// Open directory at provided path
	dir = opendir(path);

	// iterates through all files/subdirectories and increments the count
	entry = readdir(dir); 

	while (entry != NULL) {
		count += 1;

		entry = readdir(dir);
	}

	// Close the opened directory
	closedir(dir);

	return count;
}

// Compare 2 strings and return an int 
// indicating if one is alphabetically greater (>0), equal to (0), or less than (<0) the other
int compare(const void *str1, const void *str2) {
	char *const *s1 = str1;
    char *const *s2 = str2;
    return strcmp(*s1, *s2);
}

void ls(char *path, bool recurse_flag) {
	DIR *dir;
	struct dirent *entry;
	int dir_count;
	
	// Set path to current working directory if one is not provided
	if (path == NULL) {
		path = getcwd(NULL, 0);

		if (path == NULL) {
			perror("Could not access current directory.");
		}		
	}

	// Get number of elements in the directory and make an array of that size
	dir_count = get_dir_count(path);
	char *dir_elems[dir_count];

	// Determine whether to run ls recursively or not
	if (recurse_flag) {
		char *sub_path;
		struct stat st;

		// Calls non-recursive ls for current directory
		ls(path, false);

		// Open the directory at the determined path
		dir = opendir(path);

		// Iterates through the current directory and calls recursive ls for subdirectories
		entry = readdir(dir);

		while (entry != NULL) {
			// Checks that the file/directory is not itself or its parent
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
				// Gets path of subdirectory/file
				sub_path = realpath(entry->d_name, NULL);

				// Gets reference to metadata
				stat(sub_path, &st);

				// Calls recursive ls on subdirectory
				if (S_ISDIR(st.st_mode)) {
					chdir(sub_path);
					ls(sub_path, true);
					chdir("..");
				}
			}
			
			entry = readdir(dir);
		}

	} else {
		// Open the directory at the determined path
		dir = opendir(path);

		// Print current directory path
		printf("In directory: %s\n", path);
		int i = 0;
		entry = readdir(dir);

		// Read 
		while (entry != NULL) {
			dir_elems[i] = entry->d_name;
			i += 1;
			entry = readdir(dir);
		}

		// Sort dir_elems in ascending alphabetical order
		qsort(dir_elems, dir_count, sizeof(*dir_elems), compare);

		// Print out elements of dir_elems
		for (int j = 0; j < dir_count; j++) {
			if (strcmp(dir_elems[j], ".") == 0 || strcmp(dir_elems[j], "..") == 0) {
				continue;
			}
			printf("%s\n", dir_elems[j]);
		}

	}

	// Close the opened directory
	closedir(dir);
}

int main(int argc, char *argv[]){
	if(argc < 2){ // No -R flag and no path name
		ls(NULL, false);
	} else if(strcmp(argv[1], "-R") == 0) { 
		if(argc == 2) { // only -R flag
			ls(NULL, true);
		} else { // -R flag with some path name
			ls(argv[2], true);
		}
	}else { // no -R flag but path name is given
    	ls(argv[1], false);
  }
	return 0;
}
