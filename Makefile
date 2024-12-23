CC = gcc
CFLAGS = -std=gnu17 -Wall -Wextra -Werror -Wno-unused-parameter

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include
BIN_DIR = bin
TARGET = $(BIN_DIR)/edu-picodb

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all clean run

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -I $(INCLUDE_DIR) -o $@

clean:
	rm -rf $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

all: clean run