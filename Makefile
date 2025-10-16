.PHONY: clean kernel/printf.o tests/test_memory.o arch/x86/idt_asm.o

clean:
	@echo "No build artifacts to clean for the dashboard project."

kernel/printf.o:
	@echo "Kernel build step not applicable for the dashboard project."

tests/test_memory.o:
	@echo "Memory tests not applicable for the dashboard project."

arch/x86/idt_asm.o:
	@echo "Assembly step not applicable for the dashboard project."
