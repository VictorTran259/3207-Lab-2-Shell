shell: builtins.o helpers.o redirectionandpiping.o
	gcc builtins.o helpers.o redirectionandpiping.o -o shell shell.c -Wall -Werror
builtins: builtins.c
	gcc -c builtins.c
helpers: helpers.c
	gcc -c helpers.c
redirectionandpiping: redirectionandpiping.c
	gcc -c redirectionandpiping.c
clean: 
	rm  *.o shell