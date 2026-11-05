CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -pthread -Iinclude -O2
SRCS = src/common.c src/bounded_queue.c src/worker.c src/reader.c src/writer.c src/main.c
OBJS = $(SRCS:.c=.o)
TARGET = pipeline

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./run_tests.sh

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all test clean
