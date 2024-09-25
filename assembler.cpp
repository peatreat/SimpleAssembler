#include "assembler.h"
#include "single_op.h"
#include "two_op.h"
#include "jcc.h"
#include "rep.h"

bool disassemble(uint8_t* address, size_t len, ZydisDisassembledInstruction* instruction) {
	return ZYAN_SUCCESS(ZydisDisassembleIntel(ZYDIS_MACHINE_MODE_LONG_64, 0, address, len, instruction));
}

std::vector<uint8_t> Assembler::compile() {
	std::vector<RelResolver> rel_resolvers;

	ZydisDisassembledInstruction instruction;
	size_t offset = 0;

	while (offset < instructions.size()) {
		// create link with old and new
		links[offset] = this->bytecode.size();

		if (disassemble(instructions.data() + offset, instructions.size() - offset, &instruction)) {
			if (instruction.info.mnemonic == ZYDIS_MNEMONIC_NOP)
				goto loop_end;

			if (instruction.info.mnemonic == ZYDIS_MNEMONIC_RET) {
				this->push_byte(RET);
			}
			else if (instruction.info.mnemonic == ZYDIS_MNEMONIC_CPUID) {
				this->push_byte(CPUID);
			}
			else if (instruction.info.mnemonic == ZYDIS_MNEMONIC_XGETBV) {
				this->push_byte(XGETBV);
			}
			else if (instruction.info.mnemonic == ZYDIS_MNEMONIC_INT3 || instruction.info.mnemonic == ZYDIS_MNEMONIC_INT) {
				this->push_byte(INT3);
			}
			else if (instruction.info.mnemonic == ZYDIS_MNEMONIC_CDQE) {
				this->push_byte(CDQE);
			}
			else if (RepInstruction::is_rep_instruction(&instruction)) {
				RepInstruction rep_instruction;
				rep_instruction.assemble(this, &instruction, rel_resolvers, offset + instruction.info.length);
			}
			else if (JccInstruction::is_jcc_instruction(&instruction)) {
				JccInstruction jcc_instruction;
				jcc_instruction.assemble(this, &instruction, rel_resolvers, offset + instruction.info.length);
			}
			else if (instruction.info.operand_count_visible == 1) {
				SingleOpInstruction single_op_instruction;
				single_op_instruction.assemble(this, &instruction, rel_resolvers, offset + instruction.info.length);
			}
			else if (instruction.info.operand_count_visible == 2 || (instruction.info.mnemonic == ZYDIS_MNEMONIC_IMUL && instruction.info.operand_count_visible == 3)) {
				TwoOpInstruction two_op_instruction;
				two_op_instruction.assemble(this, &instruction, rel_resolvers, offset + instruction.info.length);
			}
			else {
				throw std::runtime_error("instruction not supported");
			}
		}
		else {
			this->bytecode.insert(this->bytecode.end(), instructions.begin() + offset, instructions.begin() + offset + instruction.info.length);
		}

	loop_end:
		offset += instruction.info.length;
	}

	if (!rel_resolvers.empty()) {
		printf("resolving...\n");

		for (auto& resolver : rel_resolvers) {
			resolver.resolve(this, links);
		}
	}

	/*printf("assmbled: ");

	for (uint8_t byte : this->bytecode) {
		std::cout << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)byte << ", ";
	}

	std::cout << std::endl;
	*/
	return this->bytecode;
}