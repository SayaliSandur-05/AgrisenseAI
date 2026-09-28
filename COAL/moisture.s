.intel_syntax noprefix

.global _check_moisture

_check_moisture:
    mov eax, DWORD PTR [esp+4]
    cmp eax, 30
    jl low_moisture

    mov eax, 0
    ret

low_moisture:
    mov eax, 1
    ret
    