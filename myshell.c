#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

int main() {
	char line[MAX_LINE];
	while (1) {
		printf("myshell> ");
		fflush(stdout);
		if(!fgets(line, sizeof(line), stdin))
			break;
		
		line[strcspn(line, "\n")] = 0;

		if(strlen(line) == 0)
			continue;
	}
	return 0;
}
