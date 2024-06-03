                    # =============================== #
                    # Makefile for the shell program. #
                    # =============================== #


# Setting up variables.
# ===================== #

# Compiled using GCC.
CC = gcc

# Compiler flags - warn & debug.
CFLAGS = -Wall -Wextra -g

# Target name: shell
TARGET = shell

# Source file - shell.c represents the whole shell features.
SRCS = shell.c

# Object files - shell.c --INTO--> shell.o 
OBJS = $(SRCS:.c=.o)


# Compiling & Running usage:
#    1. 'make' as default as 'make all'.
#    2. 'make run' compiles then runs the program for you.
# ======================================================== #

all: $(TARGET)

# Link the object files to create the final executable.
$(TARGET): $(OBJS)
	@echo "Linking object files to form the essence of $(TARGET)..."
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "Creation of $(TARGET) complete."

# Compile the source files into object files.
%.o: %.c shell.h
	@echo "Compiling $< into an object file..."
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compilation of $< complete."

# Run the program.
run: $(TARGET)
	@echo "Running $(TARGET)..."
	./$(TARGET)

# Clean up.
clean:
	@echo "Cleansing the workspace..."
	rm -f $(OBJS) $(TARGET)
	@echo "Workspace is now pure."

# Phony targets.
.PHONY: all clean run


                # ============================================================== #
                # This Makefile must successfully compile & run a shell program. #
                # ============================================================== #
