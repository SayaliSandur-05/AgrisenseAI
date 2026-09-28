// =============================================================================
// threshold_asm.h  --  C++ declarations for the real assembly functions
// -----------------------------------------------------------------------------
// The implementations live in threshold.asm and are linked in at build time.
// =============================================================================
#ifndef THRESHOLD_ASM_H
#define THRESHOLD_ASM_H

extern "C" {

// Returns: 0 = OK, 1 = WARNING, 2 = DANGER
int asm_soil_moisture_check(int value, int minVal, int maxVal);

// Returns: 0 = OK, 2 = DANGER
int asm_temperature_check(int value, int minVal, int maxVal);

}

#endif // THRESHOLD_ASM_H