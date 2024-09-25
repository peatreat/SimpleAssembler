#pragma once
#include <iostream>
#include "instruction.h"

class SingleOpInstruction : Instruction {
public:
    void assemble(Assembler* assembler, ZydisDisassembledInstruction* instruction, std::vector<RelResolver>& rel_resolvers, uintptr_t old_ip) {
        uint8_t opcode = 0;

        switch (instruction->info.mnemonic) {
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_PUSH, PUSH, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_POP, POP, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_CALL, CALL, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_JMP, JMP, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_INC, INC, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_DEC, DEC, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_NEG, NEG, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_SETNZ, SETNZ, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_NOT, NOT, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_SETZ, SETZ, opcode);
        default:
            throw std::runtime_error("unhandled mnemonic");
        }

        auto operand = instruction->operands[0];

        switch (operand.type) {
            case ZYDIS_OPERAND_TYPE_REGISTER: {
                assembler->add_single_op_instruction_register(opcode, RegisterID::convert_zydis_register(operand.reg.value));
                break;
            }
            case ZYDIS_OPERAND_TYPE_IMMEDIATE: {
                if (!operand.imm.is_signed && operand.size >= 32) {
                    throw std::runtime_error("immediate is unsigned");
                }

                if (operand.size > 32) {
                    throw std::runtime_error("immediate is larger than an int");
                }

                int64_t signed_imm = operand.imm.value.s;
                uint64_t unsigned_imm = operand.imm.value.u;

                if (operand.imm.is_relative) {
                    uintptr_t rip_offset_rva;
                    assembler->add_single_op_instruction_rel_offset(opcode, unsigned_imm, &rip_offset_rva);

                    auto org_instruction_rva = old_ip + signed_imm;
                    auto ip_rva = assembler->compiled_bytecode().size();

                    rel_resolvers.push_back(RelResolver(ip_rva, rip_offset_rva, org_instruction_rva, false));
                }
                else {
                    assembler->add_single_op_instruction_signed_int(opcode, unsigned_imm);
                }

                break;
            }
            case ZYDIS_OPERAND_TYPE_MEMORY: {
                int displacement = 0;

                if (operand.mem.disp.has_displacement) {
                    if (operand.mem.disp.value > INT_MAX || operand.mem.disp.value < INT_MIN)
                        throw std::runtime_error("displacement oob");

                    displacement = operand.mem.disp.value;
                }

                auto base_register = RegisterID::convert_zydis_register(operand.mem.base);

                uintptr_t displacement_offset_rva;
                assembler->add_single_op_instruction_deref(opcode, base_register, RegisterID::convert_zydis_register(operand.mem.index), displacement, operand.mem.scale, &displacement_offset_rva);

                if (base_register.index == RIP && displacement != 0) {
                    auto org_instruction_rva = old_ip + displacement;
                    auto ip_rva = assembler->compiled_bytecode().size();

                    rel_resolvers.push_back(RelResolver(ip_rva, displacement_offset_rva, org_instruction_rva, false));
                }
                break;
            }
            default:
                throw std::runtime_error("unhandled operand type");
        }
    }
};