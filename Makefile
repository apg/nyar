CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wmissing-declarations -Wno-unused-parameter

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

nyar: nyar.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm nyar *.o