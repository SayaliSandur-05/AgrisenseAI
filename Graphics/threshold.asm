; =============================================================================
; threshold.asm  --  AgriSense AI  --  Real x86-64 assembly (Windows win64 ABI)
; =============================================================================
; Assemble:  nasm -f win64 threshold.asm -o threshold.obj
; Verify:    objdump -d threshold.obj
;
; Windows x64 calling convention:
;   arg1 -> ecx,  arg2 -> edx,  arg3 -> r8d
;   return value -> eax
; =============================================================================

section .text

; -----------------------------------------------------------------------------
; int asm_soil_moisture_check(int value, int min, int max)
;   Returns: 0 = OK, 1 = WARNING (value > max), 2 = DANGER (value < min)
; -----------------------------------------------------------------------------
global asm_soil_moisture_check

asm_soil_moisture_check:
    mov     eax, ecx            ; eax = value  (arg1)
    mov     r9d, edx            ; r9d = min    (arg2)
    cmp     eax, r9d
    jge     .check_max

    mov     eax, 2              ; value < min -> DANGER
    ret

.check_max:
    mov     r9d, r8d            ; r9d = max    (arg3)
    cmp     eax, r9d
    jle     .ok

    mov     eax, 1              ; value > max -> WARNING
    ret

.ok:
    xor     eax, eax            ; return 0
    ret

; -----------------------------------------------------------------------------
; int asm_temperature_check(int value, int min, int max)
;   Returns: 0 = OK, 2 = DANGER
; -----------------------------------------------------------------------------
global asm_temperature_check

asm_temperature_check:
    mov     eax, ecx            ; value
    mov     r9d, edx            ; min
    cmp     eax, r9d
    jl      .danger

    mov     r9d, r8d            ; max
    cmp     eax, r9d
    jg      .danger

    xor     eax, eax
    ret

.danger:
    mov     eax, 2
    ret