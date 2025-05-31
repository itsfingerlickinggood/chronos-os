[bits 32]

global context_switch_asm

section .text
; void context_switch_asm(uintptr_t* prev_task_esp_storage, uintptr_t next_task_esp_value);
; Arguments are passed on the stack:
;   [esp + 4]: Address where the ESP of the old task should be stored (pointer to pcb_t->stack_pointer).
;   [esp + 8]: The ESP value of the new task to load.

context_switch_asm:
    push ebp
    mov ebp, esp

    ; Disable interrupts during the critical section of context switching.
    ; Note: EFLAGS (which includes IF) will be saved and restored as part of the context.
    ; If the new task had interrupts enabled in its saved EFLAGS, 'popf' will re-enable them.
    ; If this function is called from a scheduler that itself is part of an interrupt handler
    ; (like a timer IRQ), interrupts might already be disabled.
    ; However, an explicit CLI here ensures atomicity for the switch itself.
    cli

    ; Get arguments from the stack
    mov esi, [ebp + 8]  ; esi = prev_task_esp_storage (&prev_pcb->stack_pointer)
    mov edi, [ebp + 12] ; edi = next_task_esp_value   (next_pcb->stack_pointer value)

    ; --- Save current task's context ---
    ; The EIP is implicitly saved on the stack by the 'call context_switch_asm' instruction
    ; that invoked this function.

    ; Push EFLAGS (Interrupt Flag, etc.)
    pushf

    ; Push all general-purpose registers
    ; Order: EAX, ECX, EDX, EBX, original ESP (dummy for pushad), EBP, ESI, EDI
    ; Note: PUSHAD pushes them in a specific order, and POPAD expects this order.
    ; The order saved on stack (from low to high address) will be:
    ; EDI, ESI, EBP, ESP_dummy, EBX, EDX, ECX, EAX
    pushad

    ; Store the current stack pointer (ESP, which now points to the saved EAX)
    ; into the location provided by prev_task_esp_storage.
    ; This saves the kernel stack pointer of the outgoing task.
    mov [esi], esp

    ; --- Restore next task's context ---
    ; Load the next task's stack pointer into ESP.
    ; This ESP value should point to a previously saved context (EAX at the top).
    mov esp, edi

    ; Pop all general-purpose registers.
    ; This restores EDI, ESI, EBP, ESP_dummy, EBX, EDX, ECX, EAX from the new task's stack.
    popad

    ; Pop EFLAGS. This will restore the Interrupt Flag (IF) to whatever state
    ; it was in for the new task when it was last switched out.
    ; If IF was 1, interrupts will be enabled after this instruction (unless CLI is hit again).
    popf

    ; Restore the original EBP for this function's stack frame
    pop ebp

    ; Return. This will pop the EIP from the new task's stack (which was saved
    ; by the 'call' that switched it out, or was set up for a new task).
    ; Execution resumes at that EIP.
    ret
