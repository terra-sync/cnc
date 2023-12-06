CC = gcc
CFLAGS = -Wall -g
TARGET = cnc

SRC_DIR = src
SOURCES = $(wildcard $(SRC_DIR)/*.c)

OBJ_DIR = obj
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))

all: $(TARGET)

# Rule for creating object files directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Rule for building the target executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Generic rule for compiling any .c file into an .o file in the obj/ directory
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target for removing compiled files
clean:
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)

# Phony targets
.PHONY: all clean
