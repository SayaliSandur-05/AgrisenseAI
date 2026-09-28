.intel_syntax noprefix

# AgriSense AI - 80386 Assembly sensor range checker
#
# Each function receives three 32-bit integer arguments:
#   [esp+4]  = current value
#   [esp+8]  = minimum allowed value
#   [esp+12] = maximum allowed value
#
# Return value in EAX:
#   0 = NORMAL
#   1 = LOW
#   2 = HIGH
#
# Decimal sensor values are passed from C++ as value * 100.
# Example: pH 6.50 is passed as 650.

.global _check_moisture
.global _check_temperature
.global _check_humidity
.global _check_ph

_check_moisture:
    mov eax, DWORD PTR [esp+4]
    mov ecx, DWORD PTR [esp+8]
    mov edx, DWORD PTR [esp+12]

    cmp eax, ecx
    jl moisture_low

    cmp eax, edx
    jg moisture_high

    mov eax, 0
    ret

moisture_low:
    mov eax, 1
    ret

moisture_high:
    mov eax, 2
    ret

_check_temperature:
    mov eax, DWORD PTR [esp+4]
    mov ecx, DWORD PTR [esp+8]
    mov edx, DWORD PTR [esp+12]

    cmp eax, ecx
    jl temperature_low

    cmp eax, edx
    jg temperature_high

    mov eax, 0
    ret

temperature_low:
    mov eax, 1
    ret

temperature_high:
    mov eax, 2
    ret

_check_humidity:
    mov eax, DWORD PTR [esp+4]
    mov ecx, DWORD PTR [esp+8]
    mov edx, DWORD PTR [esp+12]

    cmp eax, ecx
    jl humidity_low

    cmp eax, edx
    jg humidity_high

    mov eax, 0
    ret

humidity_low:
    mov eax, 1
    ret

humidity_high:
    mov eax, 2
    ret

_check_ph:
    mov eax, DWORD PTR [esp+4]
    mov ecx, DWORD PTR [esp+8]
    mov edx, DWORD PTR [esp+12]

    cmp eax, ecx
    jl ph_low

    cmp eax, edx
    jg ph_high

    mov eax, 0
    ret

ph_low:
    mov eax, 1
    ret

ph_high:
    mov eax, 2
    ret
