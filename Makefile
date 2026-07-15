CC ?= cc

CPPFLAGS := -Iinclude
CFLAGS := -std=c17 -Wall -Wextra -Wpedantic -Werror -O0 -g

BUILD_DIR := build
TARGET := $(BUILD_DIR)/test_machine

CORE_SOURCES := \
	src/cpu.c \
	src/bus.c \
	src/machine.c

TEST_SOURCES := tests/test_main.c

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(CORE_SOURCES) $(TEST_SOURCES)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)