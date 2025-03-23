# Compiler and flags
CC      = gcc
CFLAGS  = -Wall -Wextra -I./src -I./tests
LDFLAGS = -lcunit

# Directories and target name
SRC_DIR    = src
BUILD_DIR  = build
TARGET     = medis
TEST_TARGET = medis_test

# Recursively find all C source files in the src directory
SOURCES := $(shell find $(SRC_DIR) -name '*.c')
# Map source files to corresponding object files in the build directory
OBJECTS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

# Recursively find all C source files in the tests directory
TEST_SOURCES := $(shell find tests -name '*.c')
# Map test source files to corresponding object files in the build directory
TEST_OBJECTS := $(patsubst tests/%.c, $(BUILD_DIR)/%.o, $(TEST_SOURCES))

# Default target: build the executable
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Pattern rule: compile any .c file into a .o file, preserving directory structure
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Pattern rule: compile any .c file into a .o file, preserving directory structure for tests
$(BUILD_DIR)/%.o: tests/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Target to build and run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS)) $(TEST_OBJECTS)
	$(CC) $(filter-out $(BUILD_DIR)/main.o, $(OBJECTS)) $(TEST_OBJECTS) -o $@ $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TEST_TARGET)

.PHONY: all clean test