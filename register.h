#pragma once
#include <iostream>

#include "Zydis.h"

#define LOBYTE(w)           ((uint8_t)(((uintptr_t)(w)) & 0xff))
#define HIBYTE(w)           ((uint8_t)((((uintptr_t)(w)) >> 8) & 0xff))

#define LODWORD(w)           ((uint32_t)(((uintptr_t)(w)) & 0xffffffff))
#define HIDWORD(w)           ((uint32_t)((((uintptr_t)(w)) >> 32) & 0xffffffff))

enum InstructionType : uint8_t {
    PUSH = 0,
    POP,
    ADD,
    SUB,
    MUL,
    DIV,
    MOV,
    LEA,
    CALL,
    RET,
    JMP,
    TEST,
    CMP,
    XOR,
    JE,
    JNE,
    JAE,
    INT3,
    STOS,
    JLE,
    MOVSX,
    INC,
    JL,
    DEC,
    MOVZX,
    AND,
    SHR,
    NEG,
    IMUL,
    CDQE,
    ROL,
    ROR,
    JNLE,
    SETNZ,
    JNBE,
    CMOVB,
    CMOVZ,
    CMOVS,
    JBE,
    JNL,
    SHL,
    NOT,
    OR,
    JB,
    CMPXCHG,
    MOVS,
    XCHG,
    SETZ,
    CMOVNZ,
    CPUID,
    XGETBV,
};

enum RegisterSize {
    LongLong = 0,
    Short = 0b1,
    Dword = 0b10,
    Byte = 0b11,
};

enum YmmRegisterSize {
    x256 = 0,
    x128 = 0b1,
};

enum Register : uint8_t {
    INVALID = 0,
    RAX,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,

    YMM0,
    YMM1,
    YMM2,
    YMM3,
    YMM4,
    YMM5,
    YMM6,
    YMM7,
    YMM8,
    YMM9,
    YMM10,
    YMM11,
    YMM12,
    YMM13,
    YMM14,
    YMM15,
 
    RIP,
};

class RegisterID {
public:
    RegisterID(uint8_t index, RegisterSize size) {
        this->index = index;
        this->size = size;
    }

    RegisterID(uint8_t index) {
        this->index = index;
        this->size = LongLong;
    }

    static RegisterID convert_zydis_register(ZydisRegister reg) {
        if (reg == ZYDIS_REGISTER_NONE) {
            return RegisterID(Register::INVALID);
        }

        if (reg == ZYDIS_REGISTER_RIP) {
            return RegisterID(Register::RIP);
        }

        if (reg >= ZYDIS_REGISTER_RAX && reg <= ZYDIS_REGISTER_R15) {
            return RegisterID(reg - ZYDIS_REGISTER_RAX + Register::RAX, LongLong);
        }

        if (reg >= ZYDIS_REGISTER_AX && reg <= ZYDIS_REGISTER_R15W) {
            return RegisterID(reg - ZYDIS_REGISTER_AX + Register::RAX, Short);
        }

        if (reg >= ZYDIS_REGISTER_EAX && reg <= ZYDIS_REGISTER_R15D) {
            return RegisterID(reg - ZYDIS_REGISTER_EAX + Register::RAX, Dword);
        }

        RegisterID converted = 0;

        switch (reg) {
        case ZYDIS_REGISTER_AL:
            converted = RegisterID(RAX, Byte);
            break;
        case ZYDIS_REGISTER_CL:
            converted = RegisterID(RCX, Byte);
            break;
        case ZYDIS_REGISTER_DL:
            converted = RegisterID(RDX, Byte);
            break;
        case ZYDIS_REGISTER_BL:
            converted = RegisterID(RBX, Byte);
            break;
        case ZYDIS_REGISTER_SPL:
            converted = RegisterID(RSP, Byte);
            break;
        case ZYDIS_REGISTER_BPL:
            converted = RegisterID(RBP, Byte);
            break;
        case ZYDIS_REGISTER_SIL:
            converted = RegisterID(RSI, Byte);
            break;
        case ZYDIS_REGISTER_DIL:
            converted = RegisterID(RDI, Byte);
            break;
        case ZYDIS_REGISTER_R8B:
        case ZYDIS_REGISTER_R9B:
        case ZYDIS_REGISTER_R10B:
        case ZYDIS_REGISTER_R11B:
        case ZYDIS_REGISTER_R12B:
        case ZYDIS_REGISTER_R13B:
        case ZYDIS_REGISTER_R14B:
        case ZYDIS_REGISTER_R15B:
            converted = RegisterID(reg - ZYDIS_REGISTER_R8B + R8, Byte);
            break;
        default:
            throw std::runtime_error("invalid zydis register");
        }

        return converted;
    }

    union {
        uint8_t value;

        struct {
            uint8_t index : 6;
            uint8_t size : 2;
        };
    };
};