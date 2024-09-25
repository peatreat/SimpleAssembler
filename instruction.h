#pragma once
#include "Zydis.h"
#include "assembler.h"
#include "rel_resolver.h"

#define TRANSLATE_ZYDIS_OPCODE(zydis_mnemonic, vm_mnemonic, output) \
case ZydisMnemonic::zydis_mnemonic: \
    output = vm_mnemonic; \
    break; \

class Instruction {
public:
	virtual void assemble(Assembler* assembler, ZydisDisassembledInstruction* instruction, std::vector<RelResolver>& rel_resolvers, uintptr_t old_ip) = 0;
};