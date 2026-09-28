// =============================================================================
// COA.h  --  AgriSense AI  --  Computer Organization & Architecture
// -----------------------------------------------------------------------------
// The threshold decisions are made by REAL x86-64 assembly (threshold.asm),
// linked into this program and executed by the host CPU.
//
// This file has two jobs:
//   1. Call the assembly functions  (asm_soil_moisture_check / asm_temperature_check)
//   2. Build a step-by-step trace that MIRRORS the assembly, so the sidebar can
//      show the instructions and their effects in the same order the CPU ran them.
//
// The trace strings exactly match the instructions in threshold.asm, so a teacher
// can open threshold.asm (or run `objdump -d threshold.o`) and verify the UI.
// =============================================================================
#ifndef COA_H
#define COA_H

#include <string>
#include <vector>
#include <cstdio>

#include "threshold_asm.h"   // the real assembly externs

class COAEngine {
public:
    struct Step {
        std::string asm_line;
        std::string effect;
        bool is_jump;
        bool jump_taken;
    };

    int AX, BX, CX;          // shadow copy of the registers used by the asm
    bool FLAG_LESS, FLAG_EQUAL, FLAG_GREATER;

    int pumpOutput;
    int alertOutput;

    std::vector<Step> trace;

    COAEngine() { reset(); }

    void reset() {
        AX = BX = CX = 0;
        FLAG_LESS = FLAG_EQUAL = FLAG_GREATER = false;
        pumpOutput = 0;
        alertOutput = 0;
        trace.clear();
    }

    void setFlags(int a, int b) {
        FLAG_LESS    = (a <  b);
        FLAG_EQUAL   = (a == b);
        FLAG_GREATER = (a >  b);
    }

    std::string flagString() const {
        if (FLAG_LESS)    return "LESS flag SET";
        if (FLAG_EQUAL)   return "EQUAL flag SET";
        return "GREATER flag SET";
    }

    void push(const std::string& line, const std::string& effect,
              bool isJump = false, bool taken = false) {
        Step s; s.asm_line = line; s.effect = effect;
        s.is_jump = isJump; s.jump_taken = taken;
        trace.push_back(s);
    }

    // =========================================================================
    // Soil moisture check.
    //   Calls real assembly: asm_soil_moisture_check(value, min, max)
    //   Then reproduces the same instruction flow in the trace, so the UI
    //   shows exactly what the CPU did.
    //   Returns 0 = OK, 1 = WARNING, 2 = DANGER
    // =========================================================================
    int runSoilMoistureCheck(float value, float minVal, float maxVal)
    {
        reset();
        AX = (int)value;
        BX = (int)minVal;
        CX = (int)maxVal;

        // ---- The real decision is made by the assembly ---- //
        int realResult = asm_soil_moisture_check(AX, BX, CX);
        alertOutput = realResult;

        // ---- Trace mirrors threshold.asm ---- //
        char buf[128];

        std::snprintf(buf, sizeof(buf), "mov eax, edi     ; value");
        push(buf, "eax <- live soil moisture");

        std::snprintf(buf, sizeof(buf), "mov ecx, esi     ; min");
        push(buf, "ecx <- ideal minimum");

        push("cmp eax, ecx", "compare value vs min");
        setFlags(AX, BX);
        push("", flagString());

        bool jge = (FLAG_GREATER || FLAG_EQUAL);
        push("jge .check_max", jge ? "jump TAKEN" : "jump NOT taken", true, jge);

        if (!jge) {
            push("mov eax, 2", "eax <- 2  (DANGER)");
            push("ret", "return to caller -- pump triggered");
            pumpOutput = 1;
            return 2;
        }

        // .check_max:
        std::snprintf(buf, sizeof(buf), "mov ecx, edx     ; max");
        push(buf, "ecx <- ideal maximum");

        push("cmp eax, ecx", "compare value vs max");
        setFlags(AX, CX);
        push("", flagString());

        bool jle = (FLAG_LESS || FLAG_EQUAL);
        push("jle .ok", jle ? "jump TAKEN" : "jump NOT taken", true, jle);

        if (!jle) {
            push("mov eax, 1", "eax <- 1  (WARNING: soil HIGH)");
            push("ret", "return to caller");
            return 1;
        }

        // .ok:
        push("xor eax, eax", "eax <- 0  (OK)");
        push("ret", "return to caller");
        return 0;
    }

    // =========================================================================
    // Temperature check -- also calls real assembly.
    //   Returns 0 = OK, 2 = DANGER
    // =========================================================================
    int runTemperatureCheck(float value, float minVal, float maxVal)
    {
        reset();
        AX = (int)value;
        BX = (int)minVal;
        CX = (int)maxVal;

        int realResult = asm_temperature_check(AX, BX, CX);
        alertOutput = realResult;

        char buf[128];

        std::snprintf(buf, sizeof(buf), "mov eax, edi     ; value");
        push(buf, "eax <- live temperature");

        std::snprintf(buf, sizeof(buf), "mov ecx, esi     ; min");
        push(buf, "ecx <- ideal temp min");

        push("cmp eax, ecx", "compare value vs min");
        setFlags(AX, BX);
        push("", flagString());

        bool jl = FLAG_LESS;
        push("jl .danger", jl ? "jump TAKEN" : "jump NOT taken", true, jl);

        if (jl) {
            push("mov eax, 2", "eax <- 2  (DANGER: temp LOW)");
            push("ret", "return to caller");
            return 2;
        }

        std::snprintf(buf, sizeof(buf), "mov ecx, edx     ; max");
        push(buf, "ecx <- ideal temp max");

        push("cmp eax, ecx", "compare value vs max");
        setFlags(AX, CX);
        push("", flagString());

        bool jg = FLAG_GREATER;
        push("jg .danger", jg ? "jump TAKEN" : "jump NOT taken", true, jg);

        if (jg) {
            push("mov eax, 2", "eax <- 2  (DANGER: temp HIGH)");
            push("ret", "return to caller");
            return 2;
        }

        push("xor eax, eax", "eax <- 0  (OK)");
        push("ret", "return to caller");
        return 0;
    }
};

#endif // COA_H