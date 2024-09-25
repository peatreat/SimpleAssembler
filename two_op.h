#pragma once
#include <iostream>
#include "instruction.h"

class TwoOpInstruction : Instruction {
    uintptr_t rip_offset_rva, org_instruction_rva;
    bool has_rip_offset;
public:
    TwoOpInstruction() {
        this->rip_offset_rva = 0;
        this->org_instruction_rva = 0;
        this->has_rip_offset = false;
    }

    void assemble(Assembler* assembler, ZydisDisassembledInstruction* instruction, std::vector<RelResolver>& rel_resolvers, uintptr_t old_ip) {
        uint8_t opcode = 0;

        switch (instruction->info.mnemonic) {
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_ADD, ADD, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_SUB, SUB, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MUL, MUL, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_DIV, DIV, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOV, MOV, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_LEA, LEA, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_TEST, TEST, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_CMP, CMP, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_XOR, XOR, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSX, MOVSX, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVSXD, MOVSX, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_MOVZX, MOVZX, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_AND, AND, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_SHR, SHR, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_IMUL, IMUL, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_ROL, ROL, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_ROR, ROR, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_CMOVB, CMOVB, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_CMOVZ, CMOVZ, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_CMOVS, CMOVS, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_SHL, SHL, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_OR, OR, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_CMPXCHG, CMPXCHG, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_XCHG, XCHG, opcode);
            TRANSLATE_ZYDIS_OPCODE(ZYDIS_MNEMONIC_CMOVNZ, CMOVNZ, opcode);
        default:
            throw std::runtime_error("unhandled mnemonic");
        }

        assembler->push_byte(opcode);

        int flags_index = assembler->compiled_bytecode().size();
        assembler->push_short(0);

        auto dst_operand = instruction->operands[0];
        auto src_operand = (opcode == IMUL) ? instruction->operands[2] : instruction->operands[1];

        if (dst_operand.mem.segment == ZYDIS_REGISTER_FS || src_operand.mem.segment == ZYDIS_REGISTER_FS)
            throw std::runtime_error("segment registers not supported yet");

        uint16_t flags = this->push_operand(assembler, &dst_operand, old_ip, true);
        flags |= this->push_operand(assembler, &src_operand, old_ip, false);

        if (this->has_rip_offset) {
            auto ip_rva = assembler->compiled_bytecode().size();
            rel_resolvers.push_back(RelResolver(ip_rva, this->rip_offset_rva, org_instruction_rva, false));
        }

        assembler->modify_short(flags_index, flags);
    }

private:
    uint16_t push_operand(Assembler* assembler, ZydisDecodedOperand* operand, uintptr_t old_ip, bool is_dst_operand) {
        switch (operand->type) {
            case ZYDIS_OPERAND_TYPE_REGISTER: {
                return assembler->add_operand_for_two_op_register(is_dst_operand, RegisterID::convert_zydis_register(operand->reg.value));
            }
            case ZYDIS_OPERAND_TYPE_IMMEDIATE: {
                if (!operand->imm.is_signed && operand->size >= 32) {
                    throw std::runtime_error("immediate is unsigned");
                }

                int64_t signed_imm = operand->imm.value.s;
                uint64_t unsigned_imm = operand->imm.value.u;

                if (operand->imm.is_relative) {
                    if (operand->size > 32) {
                        throw std::runtime_error("immediate is larger than an int");
                    }

                    this->has_rip_offset = true;
                    this->rip_offset_rva = assembler->compiled_bytecode().size();
                    this->org_instruction_rva = old_ip + signed_imm;

                    return assembler->add_operand_for_two_op_rel_offset(is_dst_operand, unsigned_imm);
                }
                else {
                    if (operand->size == 64)
                        return assembler->add_operand_for_two_op_signed_int64(is_dst_operand, unsigned_imm);
                    else if (operand->size <= 32)
                        return assembler->add_operand_for_two_op_signed_int(is_dst_operand, unsigned_imm);
                    else
                        throw std::runtime_error("unsupported immediate size");
                }
            }
            case ZYDIS_OPERAND_TYPE_MEMORY: {
                int displacement = 0;

                if (operand->mem.disp.has_displacement) {
                    if (operand->mem.disp.value > INT_MAX || operand->mem.disp.value < INT_MIN)
                        throw std::runtime_error("displacement oob");

                    displacement = operand->mem.disp.value;
                }

                auto base_register = RegisterID::convert_zydis_register(operand->mem.base);

                uintptr_t displacement_offset_rva;
                auto flags = assembler->add_operand_for_two_op_deref(is_dst_operand, base_register, RegisterID::convert_zydis_register(operand->mem.index), displacement, operand->mem.scale, &displacement_offset_rva);

                if (operand->mem.segment == ZYDIS_REGISTER_GS)
                    flags |= TwoOpFlags::HAS_SEGMENT_GS;

                if (base_register.index == RIP && displacement != 0) {
                    this->has_rip_offset = true;
                    this->rip_offset_rva = displacement_offset_rva;
                    this->org_instruction_rva = old_ip + displacement;
                }

                return flags;
            }
            default:
                throw std::runtime_error("unhandled operand type");
        }
    }
};