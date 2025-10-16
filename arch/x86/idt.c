#include "arch/x86/idt.h"
#include <stddef.h>       // For NULL, size_t
// #include "kernel/printf.h" // For kprintf, if debugging is needed later

// --- Simple memset for idt_init ---
// A proper string library would provide this.
static void* simple_memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n > 0) {
        *p++ = (unsigned char)c;
        n--;
    }
    return s;
}

// --- Static Global Variables ---
// The actual Interrupt Descriptor Table (array of entries)
static idt_entry_t idt[NUM_IDT_ENTRIES];

// The IDTR structure, which will be loaded into the IDT register.
static idt_ptr_t   idt_reg;

// --- IDT Function Implementations ---

/**
 * @brief Sets an entry in the Interrupt Descriptor Table.
 *
 * @param num The index of the IDT entry (0-255).
 * @param base The base address of the interrupt handler function.
 * @param sel The code segment selector for the handler (usually kernel code segment, e.g., 0x08).
 * @param flags The type and attribute flags for this gate descriptor
 *              (e.g., IDT_FLAG_PRESENT | IDT_FLAG_DPL_KERNEL | IDT_TYPE_32_INTERRUPT_GATE).
 */
void idt_set_gate(uint8_t num, uintptr_t base, uint16_t sel, uint8_t flags) {
    if (num >= NUM_IDT_ENTRIES) {
        // Handle error: out of bounds
        // if (kprintf) kprintf("idt_set_gate: Entry number %d out of bounds!\n", num);
        return;
    }

    idt[num].base_low  = (uint16_t)(base & 0xFFFF);         // Lower 16 bits of handler address
    idt[num].base_high = (uint16_t)((base >> 16) & 0xFFFF); // Upper 16 bits of handler address
    idt[num].selector  = sel;                               // Kernel code segment selector
    idt[num].always0   = 0;                                 // Reserved, must be zero
    idt[num].flags     = flags;                             // Type and attributes
}

/**
 * @brief Initializes the Interrupt Descriptor Table (IDT).
 *
 * This function performs the following steps:
 * 1. Clears all entries in the IDT (sets them to a default, safe state).
 * 2. Sets up the IDTR structure (`idt_reg`) with the base address and limit of the IDT.
 * 3. Calls `idt_load()` (an external assembly function) to load the IDTR into the
 *    processor's IDT register, making the IDT active.
 *
 * Note: This function only initializes the IDT structure itself. Individual interrupt
 * handlers (ISRs) must be registered using `idt_set_gate()` for each desired interrupt vector.
 * Typically, all entries are first pointed to a default "unhandled interrupt" handler.
 */

// Note: The `idt_load` function itself is expected to be in an assembly file (e.g., idt_asm.s)
// as it requires the `lidt` instruction, which is privileged and typically handled in assembly.
// The declaration `extern void idt_load(idt_ptr_t* idt_ptr);` is in `idt.h`.

// --- Include kprintf for handler output ---
#include "kernel/printf.h" // For kprintf
#include "arch/x86/pic.h"  // For pic_send_eoi
#include "arch/x86/timer.h"// For timer_handler_c

// --- PIC (Programmable Interrupt Controller) related ---
// No longer need forward declaration, pic_send_eoi is in pic.h


// --- C-Level Interrupt Handlers ---

/**
 * @brief Generic fault handler for CPU exceptions.
 *
 * This function is called from the assembly ISR stubs (isr_common_stub)
 * for CPU exceptions (ISRs 0-31). It prints register information and halts.
 * @param regs Pointer to the register state pushed onto the stack.
 */
void fault_handler(registers_t* regs) {
    kprintf("\n--- KERNEL FAULT ---\n");
    kprintf("Interrupt: %d (CPU Exception)\n", regs->int_no);
    kprintf("Error Code:  0x%x (%d)\n", regs->err_code, regs->err_code);
    kprintf("  EIP: 0x%x  CS:  0x%x  EFLAGS: 0x%x\n", regs->eip, regs->cs, regs->eflags);
    kprintf("  EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    kprintf("  ESI: 0x%x  EDI: 0x%x  EBP: 0x%x\n", regs->esi, regs->edi, regs->ebp);
    kprintf("  Original ESP (before PUSHA): 0x%x\n", regs->esp_original);
    kprintf("  Original DS: 0x%x\n", regs->original_ds);

    // Check if this fault occurred in user mode (by checking CS lowest 2 bits for CPL)
    // A non-zero value in user_ss would also indicate a privilege change.
    if (regs->cs & 0x3) { // If CPL > 0 (i.e., not kernel mode)
        kprintf("  User ESP: 0x%x  User SS: 0x%x\n", regs->user_esp, regs->user_ss);
    }

    // Specific messages for common faults
    const char* fault_messages[] = {
        "Divide-by-zero Error", "Debug", "Non-maskable Interrupt", "Breakpoint", "Overflow",
        "Bound Range Exceeded", "Invalid Opcode", "Device Not Available", "Double Fault",
        "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present", "Stack-Segment Fault",
        "General Protection Fault", "Page Fault", "Reserved", "x87 Floating-Point",
        "Alignment Check", "Machine Check", "SIMD Floating-Point", "Virtualization", "Control Protection"
    };

    if (regs->int_no < sizeof(fault_messages) / sizeof(const char*)) {
        kprintf("Description: %s\n", fault_messages[regs->int_no]);
    } else if (regs->int_no >= 22 && regs->int_no <= 31) {
        kprintf("Description: Reserved by Intel\n");
    }


    kprintf("System Halted.\n");
    asm volatile("cli; hlt"); // Disable interrupts and halt the system
}

/**
 * @brief Generic C handler for hardware IRQs.
 *
 * This function is called from the assembly IRQ stubs (irq_common_stub)
 * for hardware interrupts (ISRs 32-47). It sends an EOI to the PIC.
 * @param regs Pointer to the register state pushed onto the stack.
 */
void irq_handler_c(registers_t* regs) {
    unsigned char irq_num = (unsigned char)(regs->int_no - 32); // Calculate actual IRQ number (0-15)

    // Dispatch to specific handlers based on IRQ number
    if (irq_num == 0) { // IRQ0 is the system timer
        timer_handler_c(regs);
    } else if (irq_num == 1) { // IRQ1 is keyboard
        // keyboard_handler_c(regs); // Example for future
        // kprintf("Keyboard IRQ (1) received - no handler yet.\n");
        // For now, just acknowledge it if no specific handler, to prevent PIC lockup
    } else {
        // kprintf("Unhandled IRQ %d (INT %d) received.\n", irq_num, regs->int_no);
        // It's important to EOI even unhandled IRQs to prevent the PIC from locking up.
    }

    // Send End-Of-Interrupt (EOI) signal to the PIC(s).
    // This must be done to allow further interrupts from the PIC.
    // The pic_send_eoi function handles the master/slave logic.
    pic_send_eoi(irq_num);
}


// --- Extern declarations for ISR stubs defined in isr_stubs.s ---
// These are the entry points for each interrupt/exception.
#define DECLARE_ISR_STUB(n) extern void isr##n(void)
// ISRs 0-31 (CPU Exceptions)
DECLARE_ISR_STUB(0); DECLARE_ISR_STUB(1); DECLARE_ISR_STUB(2); DECLARE_ISR_STUB(3);
DECLARE_ISR_STUB(4); DECLARE_ISR_STUB(5); DECLARE_ISR_STUB(6); DECLARE_ISR_STUB(7);
DECLARE_ISR_STUB(8); DECLARE_ISR_STUB(9); DECLARE_ISR_STUB(10); DECLARE_ISR_STUB(11);
DECLARE_ISR_STUB(12); DECLARE_ISR_STUB(13); DECLARE_ISR_STUB(14); DECLARE_ISR_STUB(15);
DECLARE_ISR_STUB(16); DECLARE_ISR_STUB(17); DECLARE_ISR_STUB(18); DECLARE_ISR_STUB(19);
DECLARE_ISR_STUB(20); DECLARE_ISR_STUB(21); DECLARE_ISR_STUB(22); DECLARE_ISR_STUB(23);
DECLARE_ISR_STUB(24); DECLARE_ISR_STUB(25); DECLARE_ISR_STUB(26); DECLARE_ISR_STUB(27);
DECLARE_ISR_STUB(28); DECLARE_ISR_STUB(29); DECLARE_ISR_STUB(30); DECLARE_ISR_STUB(31);
// ISRs 32-47 (Hardware IRQs 0-15)
DECLARE_ISR_STUB(32); DECLARE_ISR_STUB(33); DECLARE_ISR_STUB(34); DECLARE_ISR_STUB(35);
DECLARE_ISR_STUB(36); DECLARE_ISR_STUB(37); DECLARE_ISR_STUB(38); DECLARE_ISR_STUB(39);
DECLARE_ISR_STUB(40); DECLARE_ISR_STUB(41); DECLARE_ISR_STUB(42); DECLARE_ISR_STUB(43);
DECLARE_ISR_STUB(44); DECLARE_ISR_STUB(45); DECLARE_ISR_STUB(46); DECLARE_ISR_STUB(47);


// Updated idt_init to populate with ISR/IRQ stubs
void idt_init(void) {
    simple_memset(&idt, 0, sizeof(idt_entry_t) * NUM_IDT_ENTRIES);

    // Kernel Code Segment selector (usually 0x08 after GDT setup)
    uint16_t KERNEL_CS = 0x08;

    // Table of ISR stubs for exceptions (ISRs 0-31)
    void (*isr_stub_table[])(void) = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7, isr8, isr9, isr10, isr11,
        isr12, isr13, isr14, isr15, isr16, isr17, isr18, isr19, isr20, isr21,
        isr22, isr23, isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    // Populate IDT for CPU exceptions (ISRs 0-31)
    // These are typically interrupt gates, DPL 0 (kernel mode)
    uint8_t exception_flags = IDT_FLAG_PRESENT | IDT_FLAG_DPL_KERNEL | IDT_TYPE_32_INTERRUPT_GATE;
    for (uint8_t i = 0; i < 32; ++i) {
        idt_set_gate(i, (uintptr_t)isr_stub_table[i], KERNEL_CS, exception_flags);
    }

    // Table of ISR stubs for hardware IRQs (ISRs 32-47)
    void (*irq_stub_table[])(void) = {
        isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
        isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
    };

    // Populate IDT for hardware IRQs (ISRs 32-47)
    // These are also typically interrupt gates, DPL 0
    uint8_t irq_flags = IDT_FLAG_PRESENT | IDT_FLAG_DPL_KERNEL | IDT_TYPE_32_INTERRUPT_GATE;
    for (uint8_t i = 0; i < 16; ++i) {
        idt_set_gate(32 + i, (uintptr_t)irq_stub_table[i], KERNEL_CS, irq_flags);
    }

    idt_reg.base  = (uintptr_t)&idt;
    idt_reg.limit = (sizeof(idt_entry_t) * NUM_IDT_ENTRIES) - 1;
    idt_load(&idt_reg);

    // kprintf("IDT initialized with ISR/IRQ stubs and loaded. Base: %p, Limit: 0x%x\n", idt_reg.base, idt_reg.limit);
}
