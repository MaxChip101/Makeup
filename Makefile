# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g

# Directory paths
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/objects

# Find all .c files in src directory
SRCS = $(wildcard $(SRC_DIR)/*.c)
# Generate object file names
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
# Executable name
EXEC = $(BUILD_DIR)/makeup

# Default target
all: $(BUILD_DIR) $(OBJ_DIR) $(EXEC)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Create objects directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files into executable
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC)

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Phony targets
.PHONY: all clean