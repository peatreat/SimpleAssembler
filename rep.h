#pragma once
#include <iostream>
#include "instruction.h"

class RepInstruction : Instruction {
public:
    static bool is_rep_instruction(ZydisDisassembledInstruction* instruction) {
        bool is_rep_instruction = false;

        switch (instruction->info.mnemonic) {
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSB, true, is_rep_instruction);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSW, true, is_rep_instruction);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSD, true, is_rep_instruction);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSQ, true, is_rep_instruction);

            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSB, true, is_rep_instruction);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSW, true, is_rep_instruction);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSD, true, is_rep_instruction);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSQ, true, is_rep_instruction);
        }

        return is_rep_instruction;
    }

    void set_operation_flag(ZydisDisassembledInstruction* instruction, uint8_t* flags) {
        int operation_size = -1;

        switch (instruction->info.mnemonic) {
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSB, sizeof(uint8_t), operation_size);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSW, sizeof(uint16_t), operation_size);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSD, sizeof(uint32_t), operation_size);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSQ, sizeof(uint64_t), operation_size);

            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSB, sizeof(uint8_t), operation_size);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSW, sizeof(uint16_t), operation_size);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSD, sizeof(uint32_t), operation_size);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSQ, sizeof(uint64_t), operation_size);
        }

        switch (operation_size) {
            case sizeof(uint8_t) :
                *flags |= IS_BYTE;
                break;
            case sizeof(uint16_t) :
                *flags |= IS_WORD;
                break;
            case sizeof(uint32_t) :
                *flags |= IS_DWORD;
                break;
            case sizeof(uint64_t) :
                *flags |= IS_QWORD;
                break;
            default:
                throw std::runtime_error("failed to get rep instruction operation size");
        }
    }

    void assemble(Assembler* assembler, ZydisDisassembledInstruction* instruction, std::vector<RelResolver>& rel_resolvers, uintptr_t old_ip) {
        if (instruction->info.operand_count_visible != 0) {
            throw std::runtime_error("operand count not correct");
        }

        uint8_t opcode = 0;

        switch (instruction->info.mnemonic) {
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSB, STOS, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSW, STOS, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSD, STOS, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_STOSQ, STOS, opcode);

            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSB, MOVS, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSW, MOVS, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSD, MOVS, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSQ, MOVS, opcode);
        default:
            throw std::runtime_error("unhandled mnemonic");
        }

        uint8_t flags = NO_REP;

        if (instruction->info.attributes & ZYDIS_ATTRIB_HAS_REP)
            flags |= REP;
        else if (instruction->info.attributes & ZYDIS_ATTRIB_HAS_REPE)
            flags |= REPE;
        else if (instruction->info.attributes & ZYDIS_ATTRIB_HAS_REPNE)
            flags |= REPNE;

        set_operation_flag(instruction, &flags);        

        assembler->push_byte(opcode);

        assembler->push_byte(flags);
    }
};