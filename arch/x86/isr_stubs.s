[bits 32] ; Specify 32-bit protected mode

; External C functions that these stubs will call
extern fault_handler  ; C handler for CPU exceptions/faults
extern irq_handler_c  ; C handler for hardware IRQs

; Common stub for CPU exceptions (ISRs 0-31)
isr_common_stub:
    pusha                ; Push all general purpose registers (EAX, ECX, EDX, EBX, ESP_original, EBP, ESI, EDI)
                         ; Order: EDI, ESI, EBP, ESP_original, EBX, EDX, ECX, EAX (from low to high address)

    mov eax, ds          ; Save original Data Segment (DS)
    push eax             ; Stack now: original_ds, edi, esi, ..., eax, int_no, err_code, eip, cs, eflags, [user_esp, user_ss]

    mov ebx, 0x10        ; Load kernel data segment selector (0x10 assumes GDT entry 2 is kernel data)
    mov ds, ebx          ; Set Data Segment to kernel's
    mov es, ebx          ; Set Extra Segment
    mov fs, ebx          ; Set FS
    mov gs, ebx          ; Set GS

    push esp             ; Push pointer to the current stack frame (this will be arg for C handler, points to original_ds)
    call fault_handler   ; Call the C fault handler: fault_handler(registers_t* regs)
    add esp, 4           ; Clean up the pushed ESP argument from the stack

    pop ebx              ; Restore original Data Segment from stack (where we pushed it as EAX)
    mov ds, ebx
    mov es, ebx
    mov fs, ebx
    mov gs, ebx

    popa                 ; Pop all general purpose registers
    add esp, 8           ; Clean up err_code and int_no from stack (pushed by macros/CPU)
    ; sti                ; For faults, interrupts are usually left disabled. IRET will restore original EFLAGS.
    iret                 ; Return from interrupt (pops EIP, CS, EFLAGS, and optionally UserESP, UserSS)

; Common stub for hardware interrupts (IRQs, ISRs 32-47)
irq_common_stub:
    pusha                ; Push all general purpose registers

    mov eax, ds          ; Save original Data Segment
    push eax             ; Stack: original_ds, edi, ..., eax, int_no, err_code(dummy), eip, ...

    mov ebx, 0x10        ; Load kernel data segment selector
    mov ds, ebx
    mov es, ebx
    mov fs, ebx
    mov gs, ebx

    push esp             ; Push pointer to stack frame (regs->original_ds)
    call irq_handler_c   ; Call the C IRQ handler: irq_handler_c(registers_t* regs)
    add esp, 4           ; Clean up pushed ESP

    pop ebx              ; Restore original Data Segment
    mov ds, ebx
    mov es, ebx
    mov fs, ebx
    mov gs, ebx

    popa                 ; Pop all general purpose registers
    add esp, 8           ; Clean up err_code (dummy) and int_no from stack
    sti                  ; Re-enable interrupts before returning from an IRQ
    iret                 ; Return from interrupt

; Macro for ISRs that DO NOT push an error code from the CPU
; %1: interrupt number
%macro ISR_NOERRCODE 1
    global isr%1         ; Export the symbol (e.g., isr0, isr1)
    isr%1:
        cli                  ; Disable interrupts immediately
        push byte 0          ; Push a dummy error code (for uniform stack frame)
        push byte %1         ; Push the interrupt number
        jmp isr_common_stub
%endmacro

; Macro for ISRs that DO push an error code from the CPU
; %1: interrupt number
%macro ISR_ERRCODE 1
    global isr%1         ; Export the symbol (e.g., isr8, isr10)
    isr%1:
        cli                  ; Disable interrupts immediately
        ; Error code is already on stack (pushed by CPU)
        push byte %1         ; Push the interrupt number (on top of CPU-pushed error code)
        jmp isr_common_stub
%endmacro

; Macro for IRQ handlers
; %1: IRQ number (0-15)
; %2: Corresponding ISR number (32-47)
%macro IRQ 2
    global isr%2         ; Export isrXX symbol (e.g., isr32 for IRQ 0)
    isr%2:
        cli                  ; Disable interrupts
        push byte 0          ; Push a dummy error code
        push byte %2         ; Push the ISR number (IRQ_BASE + IRQ_NUM, e.g. 32 for IRQ0)
        jmp irq_common_stub
%endmacro

; --- Define ISRs for CPU Exceptions (0-31) ---
ISR_NOERRCODE  0 ; Divide by zero
ISR_NOERRCODE  1 ; Debug
ISR_NOERRCODE  2 ; Non-Maskable Interrupt
ISR_NOERRCODE  3 ; Breakpoint
ISR_NOERRCODE  4 ; Overflow
ISR_NOERRCODE  5 ; Bound Range Exceeded
ISR_NOERRCODE  6 ; Invalid Opcode
ISR_NOERRCODE  7 ; Device Not Available (FPU/Coprocessor)
ISR_ERRCODE    8 ; Double Fault
ISR_NOERRCODE  9 ; Coprocessor Segment Overrun (obsolete)
ISR_ERRCODE   10 ; Invalid TSS (Task State Segment)
ISR_ERRCODE   11 ; Segment Not Present
ISR_ERRCODE   12 ; Stack-Segment Fault
ISR_ERRCODE   13 ; General Protection Fault
ISR_ERRCODE   14 ; Page Fault
ISR_NOERRCODE 15 ; Reserved by Intel
ISR_NOERRCODE 16 ; x87 Floating-Point Exception
ISR_ERRCODE   17 ; Alignment Check
ISR_NOERRCODE 18 ; Machine Check
ISR_NOERRCODE 19 ; SIMD Floating-Point Exception
ISR_NOERRCODE 20 ; Virtualization Exception
ISR_ERRCODE   21 ; Control Protection Exception (Intel Reserved)
; ISRs 22-31 are Intel Reserved
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; --- Define IRQ Handlers (ISRs 32-47 for IRQs 0-15) ---
; IRQ 0 (ISR 32) to IRQ 15 (ISR 47)
IRQ  0, 32  ; Timer
IRQ  1, 33  ; Keyboard
IRQ  2, 34  ; Cascade for PIC2 (Slave)
IRQ  3, 35  ; COM2
IRQ  4, 36  ; COM1
IRQ  5, 37  ; LPT2 (often sound card)
IRQ  6, 38  ; Floppy Disk
IRQ  7, 39  ; LPT1 (Parallel Port / Printer)
IRQ  8, 40  ; Real Time Clock (RTC) - Slave PIC
IRQ  9, 41  ; Free / Network Card - Slave PIC
IRQ 10, 42  ; Free / USB - Slave PIC
IRQ 11, 43  ; Free / Sound Card - Slave PIC
IRQ 12, 44  ; PS/2 Mouse - Slave PIC
IRQ 13, 45  ; FPU Coprocessor - Slave PIC
IRQ 14, 46  ; Primary ATA Hard Disk - Slave PIC
IRQ 15, 47  ; Secondary ATA Hard Disk - Slave PIC
