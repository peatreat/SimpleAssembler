#pragma once
#include <iostream>
#include "instruction.h"

class JccInstruction : Instruction {
public:
    static bool is_jcc_instruction(ZydisDisassembledInstruction* instruction) {
        bool is_jcc = false;

        switch (instruction->info.mnemonic) {
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JZ, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNZ, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNB, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JLE, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JL, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNLE, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNBE, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JBE, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNL, true, is_jcc);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JB, true, is_jcc);
        }

        return is_jcc;
    }

    void assemble(Assembler* assembler, ZydisDisassembledInstruction* instruction, std::vector<RelResolver>& rel_resolvers, uintptr_t old_ip) {
        if (instruction->info.operand_count_visible != 1) {
            throw std::runtime_error("operand count not correct");
        }

        uint8_t opcode = 0;

        switch (instruction->info.mnemonic) {
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JZ, JE, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNZ, JNE, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNB, JAE, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JLE, JLE, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JL, JL, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNLE, JNLE, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNBE, JNBE, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JBE, JBE, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JNL, JNL, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JB, JB, opcode);
        default:
            throw std::runtime_error("unhandled mnemonic");
        }

        auto operand = instruction->operands[0];

        switch (operand.type) {
        case ZYDIS_OPERAND_TYPE_IMMEDIATE: {
            if (!operand.imm.is_signed) {
                throw std::runtime_error("immediate is unsigned");
            }

            auto signed_value = operand.imm.value.s;

            if (operand.imm.is_relative) {
                assembler->push_byte(opcode);

                JccFlags flags;

                if (signed_value >= INT16_MIN && signed_value <= INT16_MAX)
                    flags = JccFlags::SIGNED_WORD;
                else if (signed_value >= INT_MIN && signed_value <= INT_MAX)
                    flags = JccFlags::SIGNED_REL_OFFSET;
                else
                    throw std::runtime_error("unsupported jcc operand size");

                assembler->push_byte(flags);

                auto rel_offset_rva = assembler->compiled_bytecode().size();
                auto org_instruction_rva = old_ip + signed_value;

                if (flags == JccFlags::SIGNED_WORD) {
                    auto ip_rva = assembler->compiled_bytecode().size() + sizeof(int16_t);
                    rel_resolvers.push_back(RelResolver(ip_rva, rel_offset_rva, org_instruction_rva, true));

                    assembler->push_short(signed_value);
                }
                else {
                    auto ip_rva = assembler->compiled_bytecode().size() + sizeof(int);
                    rel_resolvers.push_back(RelResolver(ip_rva, rel_offset_rva, org_instruction_rva, false));

                    assembler->push_int(signed_value);
                }
            }
            else {
                throw std::runtime_error("jcc operand is not relative");
            }

            break;
        }
        default:
            throw std::runtime_error("unhandled operand type");
        }
    }
};