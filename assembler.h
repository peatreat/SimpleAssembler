#pragma once
#include <iostream>
#include <vector>
#include <iomanip>
#include <map>

#include "register.h"
#include "flags.h"

class Assembler {
	std::vector<uint8_t> instructions, bytecode;

public:
	std::map<uintptr_t, uintptr_t> links;
	int64_t rebase_offset;

	Assembler(std::vector<uint8_t> instructions, int64_t rebase_offset) {
		this->instructions = instructions;
		this->rebase_offset = rebase_offset;
	}

	std::vector<uint8_t> compile();

	void add_single_op_instruction_register(uint8_t opcode, RegisterID reg) {
		uint8_t flags = SingleOpFlags::IS_REGISTER;

		this->push_byte(opcode);

		this->push_byte(flags);

		this->push_byte(reg.value);
	}

	void add_single_op_instruction_rel_offset(uint8_t opcode, int offset, uintptr_t* rip_offset_rva) {
		uint8_t flags = SingleOpFlags::IS_REL_OFFSET;

		this->push_byte(opcode);

		this->push_byte(flags);

		*rip_offset_rva = this->bytecode.size();

		this->push_int(offset);
	}

	void add_single_op_instruction_signed_int(uint8_t opcode, int signed_int) {
		uint8_t flags = SingleOpFlags::IS_SIGNED_INT;

		this->push_byte(opcode);

		this->push_byte(flags);

		this->push_int(signed_int);
	}

	void add_single_op_instruction_deref(uint8_t opcode, RegisterID base, RegisterID index, int displacement, uint8_t scale, uintptr_t* displacement_offset_rva) {
		uint8_t flags = SingleOpFlags::IS_DEREF;

		if (base.index != Register::INVALID) {
			flags |= SingleOpFlags::HAS_BASE_REG;
		}

		if (index.index != Register::INVALID) {
			flags |= SingleOpFlags::HAS_INDEX_REG;

			switch (scale) {
			case 2:
				flags |= SingleOpFlags::HAS_SCALE_MUL2;
				break;
			case 4:
				flags |= SingleOpFlags::HAS_SCALE_MUL4;
				break;
			case 8:
				flags |= SingleOpFlags::HAS_SCALE_MUL8;
				break;
			}
		}

		if (displacement != 0)
			flags |= SingleOpFlags::HAS_DISPLACEMENT;

		this->push_byte(opcode);

		this->push_byte(flags);

		if (base.index!= Register::INVALID)
			this->push_byte(base.value);

		if (index.index != Register::INVALID)
			this->push_byte(index.value);

		if (displacement != 0) {
			*displacement_offset_rva = this->bytecode.size();
			this->push_int(displacement);
		}
	}

	uint16_t add_operand_for_two_op_register(bool is_dst_operand, RegisterID reg) {
		int shift_amount = (is_dst_operand) ? 0 : 2;

		uint16_t flags = OperandType::IS_REGISTER << shift_amount;

		this->push_byte(reg.value);

		return flags;
	}

	uint16_t add_operand_for_two_op_rel_offset(bool is_dst_operand, int offset) {
		int shift_amount = (is_dst_operand) ? 0 : 2;

		uint16_t flags = OperandType::IS_REL_OFFSET << shift_amount;

		this->push_int(offset);

		return flags;
	}

	uint16_t add_operand_for_two_op_signed_int(bool is_dst_operand, int signed_int) {
		int shift_amount = (is_dst_operand) ? 0 : 2;

		uint16_t flags = OperandType::IS_SIGNED_INT << shift_amount;

		this->push_int(signed_int);

		return flags;
	}

	uint16_t add_operand_for_two_op_signed_int64(bool is_dst_operand, int64_t signed_int64) {
		int shift_amount = (is_dst_operand) ? 0 : 2;

		uint16_t flags = (OperandType::IS_SIGNED_INT << shift_amount) | TwoOpFlags::IS_IMM64;

		this->push_int64(signed_int64);

		return flags;
	}

	uint16_t add_operand_for_two_op_deref(bool is_dst_operand, RegisterID base, RegisterID index, int displacement, uint8_t scale, uintptr_t* displacement_offset_rva) {
		int shift_amount = (is_dst_operand) ? 0 : 2;

		uint16_t flags = OperandType::IS_DEREF << shift_amount;

		if (base.index != Register::INVALID) {
			flags |= TwoOpFlags::HAS_BASE_REG;
		}

		if (index.index != Register::INVALID) {
			flags |= TwoOpFlags::HAS_INDEX_REG;

			switch (scale) {
			case 2:
				flags |= TwoOpFlags::HAS_SCALE_MUL2;
				break;
			case 4:
				flags |= TwoOpFlags::HAS_SCALE_MUL4;
				break;
			case 8:
				flags |= TwoOpFlags::HAS_SCALE_MUL8;
				break;
			}
		}

		if (displacement != 0)
			flags |= TwoOpFlags::HAS_DISPLACEMENT;

		if (base.index != Register::INVALID)
			this->push_byte(base.value);

		if (index.index != Register::INVALID)
			this->push_byte(index.value);

		if (displacement != 0) {
			*displacement_offset_rva = this->bytecode.size();
			this->push_int(displacement);
		}

		return flags;
	}

	const std::vector<uint8_t>& compiled_bytecode() {
		return this->bytecode;
	}

	std::vector<uint8_t>* compiled_bytecode_mut() {
		return &this->bytecode;
	}

	void push_byte(uint8_t byte) {
		bytecode.push_back(byte);
	}

	void push_short(uint16_t v) {
		bytecode.push_back(LOBYTE(v));
		bytecode.push_back(HIBYTE(v));
	}

	void push_int(int v) {
		bytecode.push_back(LOBYTE(v));
		bytecode.push_back(LOBYTE(v >> 8));
		bytecode.push_back(LOBYTE(v >> 16));
		bytecode.push_back(HIBYTE(v >> 16));
	}

	void push_int64(int64_t v) {
		this->push_int(LODWORD(v));
		this->push_int(HIDWORD(v));
	}

	void modify_short(int index, uint16_t new_value) {
		*(uint16_t*)&bytecode[index] = new_value;
	}
};