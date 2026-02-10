myshell : myshell.c
	@gcc myshell.c -o myshell -Wall

clean:
	@rm -f myshell
