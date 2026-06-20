CC = gcc
CFLAGS = -Wall -Wextra -g

malloc-r.bin:
	$(CC) $(CFLAGS) main.c -o malloc-r.bin


