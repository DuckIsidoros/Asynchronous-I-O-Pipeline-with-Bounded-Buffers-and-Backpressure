
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -pthread -Iinclude
LDFLAGS := -pthread

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
DEP := $(OBJ:.o=.d)
TARGET := build/pipeline

.PHONY: all objects clean

all: $(TARGET)

objects: $(OBJ)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf build

-include $(DEP)
