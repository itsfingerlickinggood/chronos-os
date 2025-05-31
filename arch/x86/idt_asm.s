; File: arch/x86/idt_asm.s
; Purpose: Contains the idt_load function to load the IDT register (IDTR).

section .text
bits 32         ; We are in 32-bit protected mode

global idt_load ; Makes 'idt_load' visible to C code

; void idt_load(idt_ptr_t* idt_ptr);
; The argument (pointer to idt_ptr_t structure) is passed on the stack.
idt_load:
    push ebp          ; Set up stack frame
    mov ebp, esp

    mov eax, [ebp + 8] ; Get the argument (idt_ptr_t*) from the stack.
                      ; [ebp+4] is the return address, [ebp+8] is the first argument.

    lidt [eax]        ; Load the IDT register with the contents pointed to by EAX.
                      ; EAX holds the address of our idt_ptr_t structure.

    pop ebp           ; Restore stack frame
    ret               ; Return from function
