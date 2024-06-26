SRC_FILES = $(filter-out instruction-test.c, $(wildcard *.c))
TEST_SRC_FILES = instruction-test.c types.c trim.c utils.c

all:
	gcc -o arabica-compiler $(SRC_FILES) -Wall -Wextra -Werror

clean:
	rm -f arabica-compiler.exe & rm -f arabica-compiler
	rm -f arabica-test.exe & rm -f arabica-test

fclean:
	rm -f arabica-compiler.exe & rm -f arabica-compiler
	rm -f arabica-test.exe & rm -f arabica-test

test:
	gcc -o arabica-test $(TEST_SRC_FILES) -Wall -Wextra -Werror
	./arabica-test