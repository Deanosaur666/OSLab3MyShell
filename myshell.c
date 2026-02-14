// this is a lot of includes
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

// how many characters can a user enter in one line?
#define LINE_MAXCHARS 1024
// how many arguments can one command have?
#define MAX_ARGS 16
// how long can one argument be?
#define ARG_MAXCHAR 128

#define STRING_CHAR_INDEX(s, c) (strchr(s, c) == NULL ? -1 : (int)(strchr(s, c) - s))

// what type of redirection are we doing?
typedef enum RedirectionType {
	REDIR_NONE,		// no redirection
	REDIR_OUTPUT,	// >
	REDIR_APPEND,	// >>
	REDIR_INPUT,	// <

	REDIR_ERROR, // if the user tries to use more than one redirection
} RedirectionType;

int main() {
	// the line the user inputs
	char line[LINE_MAXCHARS];
	while (1) {
		printf("myshell> ");
		fflush(stdout); // not totally sure why this is so important, just copied from example
		// fgets to read user input
		if(!fgets(line, sizeof(line), stdin)) // this terminates on error or end of file
			break;
		
		// the line ends in a newline char
		int lineLen = STRING_CHAR_INDEX(line, '\n');
		// we replace the last char with a null character
		line[lineLen] = 0;

		// if we received an empty line, do nothing
		if(strlen(line) == 0)
			continue;
		
		// if the user typed exit, exit the program
		if(strcmp(line, "exit") == 0)
			break;
		
		// tokenize arguments
		// this string array is probably oversized, but better to avoid malloc if possible
		char args[MAX_ARGS][ARG_MAXCHAR] = { 0 };

		int argCount = 0;
		RedirectionType redir = REDIR_NONE;
		int redirFileArgIndex = 0; // where is the file we're redirecting to/from in the array?
		char * lineSub = line; // substring for scanning forward through the line
		// if we hit the end of the line, or hit max args, abort
		while((int)(lineSub - line) < lineLen && argCount < MAX_ARGS) {
			int argEnd;
			bool quote = false;
			if(lineSub[0] == 0) {
				break;
			}
			// input redirection
			if(lineSub[0] == '<') {
				if(redir != REDIR_NONE) {
					redir = REDIR_ERROR;
					break; // can't have multiple redirections
				}
				argEnd = 1;
				redir = REDIR_INPUT;
				redirFileArgIndex = argCount + 1; // next argument is redirection location
			}
			// output redirection
			else if(lineSub[0] == '>') {
				if(redir != REDIR_NONE) {
					redir = REDIR_ERROR;
					break; // can't have multiple redirections
				}
				// append
				if((int)(lineSub - line) < lineLen-1 && lineSub[1] == '>') {
					argEnd = 2;
					redir = REDIR_APPEND;
					redirFileArgIndex = argCount + 1; // next argument is redirection location
				}
				// clobber
				else {
					argEnd = 1;
					redir = REDIR_OUTPUT;
					redirFileArgIndex = argCount + 1; // next argument is redirection location
				}
			}
			// double quote string
			else if(lineSub[0] == '\"') {
				lineSub ++; // don't include starting quote
				argEnd = STRING_CHAR_INDEX(lineSub, '\"');
				quote = true;
			}
			// single quote string
			else if(lineSub[0] == '\'') {
				lineSub ++; // don't include starting quote
				argEnd = STRING_CHAR_INDEX(lineSub, '\'');
				quote = true;
			}
			// unquoted arg
			// end on space, tab, quote, or redirection
			else
				argEnd = strcspn(lineSub, " \t\"\'><");
			// if we don't find an ending character, we go till the end of the line
			if(argEnd == -1)
				argEnd = lineLen;
			
			// get substring using strncpy
			strncpy(args[argCount], lineSub, argEnd);
			argCount ++;
			
			lineSub += argEnd;
			// we skip an end quote
			if(quote)
				lineSub ++;
			// skip over multiple spaces or tabs
			while((int)(lineSub - line) < lineLen && strcspn(lineSub, " \t\0") == 0) {
				lineSub ++;
			}
		}

		// cd is built in
		if(strcmp(args[0], "cd") == 0) {
			if(argCount > 2)
				printf("Error. Too many arguments for cd.\n");
			// cd
			else if(argCount == 1)
				chdir(getenv("HOME"));
			// cd /whatever
			else {
				char* dirName = args[1];
				DIR* dir = opendir(dirName);
				if(dir) {
					chdir(dirName);
					closedir(dir);
				}
				else {
					printf("Error. Directory does not exist.\n");
				}
			}
		}

		// multiple redirections
		else if(redir == REDIR_ERROR) {
			printf("Error. This shell does not support more than one redirection per command.\n");
		}
		// redirection symbol at end of line
		else if(redir != REDIR_NONE && redirFileArgIndex >= argCount) {
			printf("Error. Redirection file missing.\n");
		}
		// input file does not exist
		else if(redir == REDIR_INPUT && access(args[redirFileArgIndex], F_OK) != 0) {
			printf("Error. Redirection file does not exist.\n");
		}
		// we're good to execute a command
		else {
			char * redirFile = NULL;
			if(redir != REDIR_NONE) {
				argCount = redirFileArgIndex - 1;
				redirFile = args[redirFileArgIndex];
			}
			// we have the arguments now
			// convert to format used by execvp
			char * argStrings[MAX_ARGS] = { NULL }; // full of nulls
			for(int i = 0; i < argCount; i ++) {
				argStrings[i] = args[i];
			}

			// we fork and execute now
			pid_t pid = fork();
			// child
			if(pid == 0) {
				if(redir == REDIR_OUTPUT) {
					int fd = open(redirFile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
					dup2(fd, STDOUT_FILENO);
        			close(fd);
				}
				else if(redir == REDIR_APPEND) {
					int fd = open(redirFile, O_WRONLY|O_CREAT|O_APPEND, 0666);
					dup2(fd, STDOUT_FILENO);
        			close(fd);
				}
				else if(redir == REDIR_INPUT) {
					int fd = open(redirFile, O_RDONLY, 0666);
					dup2(fd, STDIN_FILENO);
					close(fd);
				}
				// execute command
				return execvp(argStrings[0], argStrings);
			}
			// parent
			else {
				int status;
				// wait until child is finished
				waitpid(pid, &status, 0);
				// exit status
				if(WIFEXITED(status) && WEXITSTATUS(status)) {
					printf("Command failed. Error code %d\n", WEXITSTATUS(status));
				}
			}
		}
		
		// loop back and read the next line
	}
	return 0;
}
