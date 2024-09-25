#pragma once
#include "assembler.h"
#include <iostream>
#include <map>

class RelResolver {
	uintptr_t ip_rva;
	uintptr_t rel_offset_rva;
	uintptr_t org_instruction_it_pointed_to_rva;
	bool is_word_rel_offset;

public:
	RelResolver(uintptr_t ip_rva, uintptr_t rel_offset_rva, uintptr_t org_instruction_rva, bool is_byte_rel_offset) {
		this->ip_rva = ip_rva;
		this->rel_offset_rva = rel_offset_rva;
		this->org_instruction_it_pointed_to_rva = org_instruction_rva;
		this->is_word_rel_offset = is_byte_rel_offset;
	}

	void resolve(Assembler* assembler, std::map<uintptr_t, uintptr_t>& link_map) {
		auto new_instruction_rva = link_map.find(this->org_instruction_it_pointed_to_rva);
		int64_t rip_offset;

		// attempt to fix rip offset by checking list map
		if (new_instruction_rva != link_map.end())
			rip_offset = new_instruction_rva->second - this->ip_rva;
		else { // fix for rva outside of instruction segment
			if (this->org_instruction_it_pointed_to_rva < assembler->rebase_offset || this->org_instruction_it_pointed_to_rva >= assembler->rebase_offset + assembler->compiled_bytecode().size()) {
				rip_offset = this->org_instruction_it_pointed_to_rva - (assembler->rebase_offset + this->ip_rva);
			}
			else {
				throw std::runtime_error("failed to resolve rip offset");
			}
		}

		if (rip_offset > INT_MAX || rip_offset < INT_MIN) {
			throw std::runtime_error("new rip offset is oob");
		}

		if ((rip_offset > INT16_MAX || rip_offset < INT16_MIN) && is_word_rel_offset) {
			throw std::runtime_error("new rip offset is oob");
		}

		auto rel_offset = (int*)&assembler->compiled_bytecode_mut()->data()[this->rel_offset_rva];

		if (is_word_rel_offset) {
			*(int16_t*)rel_offset = *(int16_t*)&rip_offset;
		}
		else {
			*rel_offset = *(int*)&rip_offset;
		}
	}
};