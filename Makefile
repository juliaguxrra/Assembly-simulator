.PHONY: clean
CC=gcc
CFLAGS=-I. -g -Wall --std=gnu11 -fpic
SRCS=machine.c code.c
PROGRAM=simulator
TESTS=test_operands

all: $(PROGRAM) $(TESTS) lib$(PROGRAM).so

%: $(SRCS) %.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAM) $(TESTS) lib$(PROGRAM).so

lib%.so: $(SRCS)
	$(CC) $(CFLAGS) -shared -o $@ $^