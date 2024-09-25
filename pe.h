#pragma once
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "assembler.h"

class PeFile {
	std::vector<uint8_t> file;
public:
	PeFile(std::string filepath) {
		std::ifstream file(filepath, std::ios::in | std::ios::binary);

		if (!file.is_open())
			throw std::runtime_error("failed to open " + filepath);

		auto beg = file.tellg();
		file.seekg(0, std::ios::end);
		auto filesize = file.tellg() - beg;
		file.seekg(0, std::ios::beg);

		this->file.resize(filesize);
		file.read((char*)this->file.data(), filesize);

		file.close();
	}

	PIMAGE_DOS_HEADER get_dos() {
		return PIMAGE_DOS_HEADER(file.data());
	}

	PIMAGE_NT_HEADERS get_nt() {
		return PIMAGE_NT_HEADERS(this->file.data() + this->get_dos()->e_lfanew);
	}

	PIMAGE_SECTION_HEADER get_first_section() {
		return IMAGE_FIRST_SECTION(this->get_nt());
	}

	PIMAGE_SECTION_HEADER create_new_section(char name[8], DWORD characteristics) {
		auto new_section = this->get_first_section() + this->get_nt()->FileHeader.NumberOfSections;

		*(uint64_t*)new_section->Name = *(uint64_t*)name;;
		new_section->VirtualAddress = (new_section - 1)->VirtualAddress + (new_section - 1)->Misc.VirtualSize;

		if (new_section->VirtualAddress & 0xFFF) {
			new_section->VirtualAddress += 0x1000;
			new_section->VirtualAddress &= ~0xFFF;
		}

		new_section->Characteristics = characteristics;

		return new_section;
	}

	void write_section_data(PIMAGE_SECTION_HEADER section, std::vector<uint8_t>& data) {
		section->PointerToRawData = (section - 1)->PointerToRawData + (section - 1)->SizeOfRawData;
		section->SizeOfRawData = data.size();
		section->Misc.VirtualSize = data.size();

		if (section->Misc.VirtualSize & 0xFFF) {
			section->Misc.VirtualSize += 0x1000;
			section->Misc.VirtualSize &= ~0xFFF;
		}

		if (section->PointerToRawData + data.size() >= this->file.size()) {
			this->file.insert(this->file.begin() + section->PointerToRawData, data.begin(), data.end());
		}
		else {
			memcpy(this->file.data() + section->PointerToRawData, &data[0], data.size());
		}
	}

	std::vector<uint8_t> virtualize() {
		auto section = this->get_first_section();

		int added = 0;
		
		for (int i = 0; i < this->get_nt()->FileHeader.NumberOfSections; section++, i++) {
			if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE && (section->Characteristics & IMAGE_SCN_CNT_UNINITIALIZED_DATA) == 0) {
				std::vector<uint8_t> instructions;
				instructions.assign(this->file.data() + section->PointerToRawData, this->file.data() + section->PointerToRawData + section->SizeOfRawData);

				auto new_section = this->create_new_section((char*)(std::string(".pvm") + std::to_string(added)).c_str(), section->Characteristics & ~IMAGE_SCN_MEM_EXECUTE);

				get_nt()->FileHeader.NumberOfSections++;
				added++;

				Assembler assembler(instructions, new_section->VirtualAddress - section->VirtualAddress);

				try {
					auto compiled = assembler.compile();

					if (get_nt()->OptionalHeader.AddressOfEntryPoint >= section->VirtualAddress && get_nt()->OptionalHeader.AddressOfEntryPoint < section->VirtualAddress + section->Misc.VirtualSize) {
						printf("old entrypoint: %p\n", get_nt()->OptionalHeader.AddressOfEntryPoint);
						get_nt()->OptionalHeader.AddressOfEntryPoint = new_section->VirtualAddress + assembler.links[get_nt()->OptionalHeader.AddressOfEntryPoint - section->VirtualAddress];
						printf("new entrypoint: %p\n", get_nt()->OptionalHeader.AddressOfEntryPoint);
					}

					this->write_section_data(new_section, compiled);

					get_nt()->OptionalHeader.SizeOfImage += new_section->Misc.VirtualSize;

					std::ofstream translations("translations.txt");

					for (auto& link : assembler.links)
						translations << "0x" << std::hex << link.first << " -> 0x" << std::hex << link.second << std::endl;

					translations.close();
				}
				catch (std::runtime_error error) {
					printf("runtime error: %s\n", error.what());
				}
			}
		}

		return this->file;
	}
};