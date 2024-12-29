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
	./$(TARGET) -f test_file -n -s "(my_col1:int _my_col:float mycol:string)"
	./$(TARGET) -f test_file -a "(123 && 1.4 && Hello world!)"

all: clean run

all-verify: clean
	@echo "Compiling with VERIFY_HEADER"
	$(MAKE) CFLAGS="$(CFLAGS) -DVERIFY_HEADER" all

verify-row: clean
	@echo "Compiling with VERIFY_ROW"
	$(MAKE) CFLAGS="$(CFLAGS) -DVERIFY_ROW" all