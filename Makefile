all:
	gcc -o arabica *.c -Wall -Wextra -Werror

clean:
	rm -f arabica.exe & rm -f arabica

fclean:
	rm -f arabica.exe & rm -f arabica

run: all
	./arabica
