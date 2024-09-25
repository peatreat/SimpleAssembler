#pragma once
#include <iostream>

enum REPFlags : uint8_t {
	NO_REP = 0,
	REP = 0b1,
	REPE = 0b10,
	REPNE = 0b11,

	IS_BYTE = 0b100,
	IS_WORD = 0b1000,
	IS_DWORD = 0b10000,
	IS_QWORD = 0b100000,
};

enum JccFlags : uint8_t {
	SIGNED_WORD = 0,
	SIGNED_REL_OFFSET = 0b1,
};

class SingleOpFlags {
public:
    enum : uint8_t {
        IS_DEREF = 0,
        IS_REGISTER = 0b1,
        IS_REL_OFFSET = 0b10,
        IS_SIGNED_INT = 0b11,

        HAS_DISPLACEMENT = 0b100,
        HAS_BASE_REG = 0b1000,
        HAS_INDEX_REG = 0b10000,
        HAS_SCALE_MUL2 = 0b100000,
        HAS_SCALE_MUL4 = 0b1000000,
        HAS_SCALE_MUL8 = 0b1100000,
    };
};

class TwoOpFlags {
public:
	enum : uint16_t {
		DST_IS_REGISTER = 0b1,
		DST_IS_REL_OFFSET = 0b10,
		SRC_IS_REGISTER = 0b100,
		SRC_IS_IS_REL_OFFSET = 0b1000,
		SRC_IS_SIGNED_INT = 0b1100,
		IS_IMM64 = 0b10000,

		HAS_SEGMENT_GS = 0b100000,
		HAS_DISPLACEMENT = 0b1000000,
		HAS_BASE_REG = 0b10000000,
		HAS_INDEX_REG = 0b100000000,
		HAS_SCALE_MUL2 = 0b1000000000,
		HAS_SCALE_MUL4 = 0b10000000000,
		HAS_SCALE_MUL8 = 0b11000000000,
	};
};

enum OperandType : uint8_t {
	IS_DEREF = 0,
	IS_REGISTER = 0b1,
	IS_REL_OFFSET = 0b10,
	IS_SIGNED_INT = 0b11,
};