#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAXCHARS 1024
#define MAX_ARGS 16
#define ARG_MAXCHAR 128

#define STRING_CHAR_INDEX(s, c) (strchr(s, c) == NULL ? -1 : (int)(strchr(s, c) - s))

int main() {
	// the line the user inputs
	char line[LINE_MAXCHARS];
	while (1) {
		printf("myshell> ");
		fflush(stdout); // not totally sure why this is so important, just copied from example
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
		char * lineSub = line;
		while((int)(lineSub - line) < lineLen && argCount < MAX_ARGS) {
			int argEnd;
			if(lineSub[0] == '\"') {
				lineSub ++; // don't include starting quote
				argEnd = STRING_CHAR_INDEX(lineSub, '\"');
			}
			else
				argEnd = strcspn(lineSub, " \t");
			if(argEnd == -1)
				argEnd = lineLen;
			
			// get substring using strncpy
			strncpy(args[argCount], lineSub, argEnd);
			argCount ++;
			
			lineSub += argEnd + 1;
			while((int)(lineSub - line) < lineLen && strcspn(lineSub, " \t") == 0) {
				lineSub ++;
			}
		}

		// we have the arguments now
		// convert to format used by execvp
		char * argStrings[MAX_ARGS] = { NULL }; // full of nulls
		for(int i = 0; i < argCount; i ++) {
			argStrings[i] = args[i];
			//printf("%s\n", args[i]);
		}

		pid_t pid = fork();
		if(pid == 0) {
			return execvp(argStrings[0], argStrings);
		}
		else {
			int status;
            waitpid(pid, &status, 0);
			// exit status
            if(WIFEXITED(status) && WEXITSTATUS(status)) {
                printf("Command failed. Error code %d\n", WEXITSTATUS(status));
            }
		}
		
	}
	return 0;
}
