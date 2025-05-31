#ifndef _ARCH_X86_IDT_H
#define _ARCH_X86_IDT_H

#include <stdint.h> // For uint8_t, uint16_t, uint32_t, uintptr_t

// --- IDT Entry Structure (Gate Descriptor) ---
// Ref: Intel Manual Vol 3a, Section 6.11 Interrupt Descriptor Table.
// This structure must be packed.
struct idt_entry {
    uint16_t base_low;    // Lower 16 bits of the ISR's address
    uint16_t selector;    // Kernel code segment selector (e.g., 0x08)
    uint8_t  always0;     // This byte must always be zero (reserved)
    uint8_t  flags;       // Type and attributes (P, DPL, S, Type fields)
    uint16_t base_high;   // Upper 16 bits of the ISR's address
} __attribute__((packed));
typedef struct idt_entry idt_entry_t;

// --- IDT Pointer Structure (IDTR) ---
// Ref: Intel Manual Vol 3a, Section 2.4.1 IDTR Interrupt Descriptor Table Register.
// This structure must be packed.
struct idt_ptr {
    uint16_t limit;       // Size of IDT in bytes minus 1 (e.g., 256 * 8 - 1)
    uintptr_t base;       // Linear base address of the IDT
} __attribute__((packed));
typedef struct idt_ptr idt_ptr_t;

// --- Constants ---
#define NUM_IDT_ENTRIES 256 // Standard number of IDT entries

// --- Flags for idt_entry_t.flags (Type and Attributes byte) ---
// Ref: Intel Manual Vol 3a, Figure 6-2. IDT Gate Descriptors.
// The 'flags' byte is structured as: P | DPL(2) | S | Type(4)
// P: Present bit (1 if segment is present)
// DPL: Descriptor Privilege Level (00 for kernel, 11 for user)
// S: Storage Segment bit (0 for interrupt and trap gates, 1 for task gates)
// Type: Gate type (e.g., 32-bit interrupt gate, 32-bit trap gate)

#define IDT_FLAG_PRESENT             (1 << 7) // Bit 7: Present (P)

// Descriptor Privilege Level (DPL) - bits 5-6
#define IDT_FLAG_DPL_KERNEL          (0 << 5) // Ring 0 (Kernel)
#define IDT_FLAG_DPL_USER            (3 << 5) // Ring 3 (User)

// Storage Segment (S) - bit 4
// This bit should be 0 for Interrupt Gates and Trap Gates.
#define IDT_FLAG_SYSTEM_DESCRIPTOR   (0 << 4) // For Interrupt & Trap gates
#define IDT_FLAG_STORAGE_DESCRIPTOR  (1 << 4) // For Task gates (not typically used for ISRs)


// Gate Types - lower 4 bits (0-3)
// Common types for 32-bit protected mode:
#define IDT_TYPE_TASK_GATE           0x05 // Task Gate (rarely used for ISRs)
#define IDT_TYPE_16_INTERRUPT_GATE   0x06 // 16-bit Interrupt Gate
#define IDT_TYPE_16_TRAP_GATE        0x07 // 16-bit Trap Gate
#define IDT_TYPE_32_INTERRUPT_GATE   0x0E // 32-bit Interrupt Gate (interrupts disabled when handler called)
#define IDT_TYPE_32_TRAP_GATE        0x0F // 32-bit Trap Gate (interrupts remain enabled)


// --- Register Structure for Interrupt/Exception Handlers ---

// This structure defines the layout of registers pushed onto the stack by:
// 1. The processor automatically (EIP, CS, EFLAGS, optionally UserESP, UserSS).
// 2. Our assembly ISR stubs (error code (or dummy), interrupt number, general-purpose registers, data segment).
// The C handler (e.g., fault_handler, irq_handler_c) receives a pointer to this structure.
// The order of fields is critical and must match the order of pushes in assembly.
typedef struct {
    // Pushed by our assembly stub (isr_common_stub or irq_common_stub) after PUSHA
    uint32_t original_ds; // Original Data Segment (DS, ES, FS, GS are set to kernel DS by stub)

    // Pushed by PUSHA instruction (from lowest address on stack to highest for this block)
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_original; // Original ESP value before PUSHA instruction (dummy value from PUSHA)
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    // Pushed by our assembly stub (isrXXX or irqYYY macro)
    uint32_t int_no;       // Interrupt number

    // Pushed by CPU for certain exceptions, or a dummy 0 by our stub for others/IRQs
    uint32_t err_code;

    // Pushed by CPU automatically on interrupt/exception
    uint32_t eip;          // Instruction Pointer of the interrupted code
    uint32_t cs;           // Code Segment of the interrupted code
    uint32_t eflags;       // Processor EFLAGS register before interrupt
    uint32_t user_esp;     // Stack pointer (if privilege change CPL0->CPL3 to CPL0)
    uint32_t user_ss;      // Stack segment (if privilege change CPL0->CPL3 to CPL0)
} __attribute__((packed)) registers_t;


// --- Function Declarations ---

// C-level interrupt handlers
void fault_handler(registers_t* regs); // For CPU exceptions/faults
void irq_handler_c(registers_t* regs); // For hardware IRQs

// Assembly function for context switching
extern void context_switch_asm(uintptr_t* prev_task_esp_storage, uintptr_t next_task_esp_value);

/**
 * @brief Sets an entry in the Interrupt Descriptor Table.
 *
 * @param num The index of the IDT entry (0-255).
 * @param base The base address of the interrupt handler function.
 * @param sel The code segment selector for the handler (usually kernel code segment).
 * @param flags The type and attribute flags for this gate descriptor.
 */
void idt_set_gate(uint8_t num, uintptr_t base, uint16_t sel, uint8_t flags);

/**
 * @brief Initializes the Interrupt Descriptor Table.
 * Sets up all entries (e.g., to default handlers) and loads the IDTR.
 */
void idt_init(void);

/**
 * @brief Loads the IDT register (IDTR) with the address and limit of the IDT.
 * This function is typically implemented in assembly (`lidt` instruction).
 *
 * @param idt_ptr Pointer to the idt_ptr_t structure.
 */
extern void idt_load(idt_ptr_t* idt_ptr); // Usually in an assembly file (e.g., idt_asm.s)


// Example of combining flags for a 32-bit kernel-mode interrupt gate:
// uint8_t example_flags = IDT_FLAG_PRESENT | IDT_FLAG_DPL_KERNEL | IDT_FLAG_SYSTEM_DESCRIPTOR | IDT_TYPE_32_INTERRUPT_GATE;
// Note: IDT_FLAG_SYSTEM_DESCRIPTOR implies S=0. Some people might define IDT_FLAG_INTERRUPT_GATE which combines S=0 and Type.

#endif // _ARCH_X86_IDT_H
