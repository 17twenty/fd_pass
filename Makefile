# Simple demo of passing file descriptors
# ./i_use_pid & ./i_print_pid

all:
	$(CROSS_COMPILE)-$(CC) -o i_use_pid i_use_pid.c
	$(CROSS_COMPILE)-$(CC) -o i_print_pid i_print_pid.c

.PHONY: clean
clean:
	-@rm i_use_pid i_print_pid my_file.txt
