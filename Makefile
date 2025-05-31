# Compiler and Compiler Flags
CC = gcc
CFLAGS = -Wall -Wextra -nostdlib -fno-builtin -Iinclude -c
LDFLAGS =

# Assembler and Assembler Flags
ASM = nasm
ASMFLAGS = -f elf32 # Output format: ELF32

# Source Files
SRCS = kernel/main.c kernel/vga.c kernel/printf.c kernel/scheduler.c kernel/memory.c \
       kernel/ai_core.c tests/test_memory.c \
       arch/x86/idt.c arch/x86/pic.c

ASMSOURCES = arch/x86/idt_asm.s arch/x86/isr_stubs.s

# Object Files
OBJS = $(SRCS:.c=.o) $(ASMSOURCES:.s=.o)

# Target Executable
TARGET = kernel.bin

# Default Target
all: $(TARGET)

# Link object files to create the target executable
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(TARGET)

# Compile C source files into object files
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# Compile Assembly source files into object files
%.o: %.s
	$(ASM) $(ASMFLAGS) $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
