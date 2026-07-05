CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = -o malloc-r.bin main.c

malloc-r.bin:
	$(CC) $(CFLAGS) $(TARGET)
	
clean:
	rm -rf malloc-r.bin


