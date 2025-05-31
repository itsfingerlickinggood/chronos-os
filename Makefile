# Compiler and Compiler Flags
CC = gcc
CFLAGS = -Wall -Wextra -nostdlib -fno-builtin -Iinclude -c
LDFLAGS =

# Source and Object Files
SRCS = kernel/main.c kernel/scheduler.c kernel/memory.c kernel/ai_core.c tests/test_memory.c
OBJS = $(SRCS:.c=.o)

# Target Executable
TARGET = kernel.bin

# Default Target
all: $(TARGET)

# Link object files to create the target executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(TARGET)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
