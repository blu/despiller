#if !defined(__isa_h)
#define __isa_h

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

// Fake ISA whose sole purpose is to demonstrate the effect of de-spilling

// This ISA of little-endian 31-bit machine word has an unspecified-size GPR file R0..Rn, and an unlimited storage space 'storage'
// where regs can be spilled -- i.e. stored to and eventually restored from, in a LIFO manner

namespace isa {

// opcodes
enum Opcode : uint8_t {
	op_nop,         // no-op: nop
	op_li,          // load immediate to register: li Rn, imm
	op_push,        // store a single register to 'storage': push Rn
	op_pop,         // restore a single register from 'storage': pop Rn
	op_br,          // unconditional branch to register: br Rt
	op_cbr,         // conditional branch to register: cbr Rt, Rn, Rm ; semantics: branch to Rt if unspecified comparison between Rn and Rm is true
	op_op2,         // unspecified op which is not any of the above, taking 2 operands -- one destination and one source: op Rn, Rm
	op_op3,         // unspecified op which is not any of the above, taking 3 operands -- one destination and two sources: op Rn, Rm, Rk

	op__count
};

// operands -- usually register identifiers
typedef uint8_t Operand;

constexpr Opcode op_invalid = Opcode(uint8_t(-1));
constexpr Operand reg_invalid = Operand(-1);

// check opcode validity
inline bool isOpcodeValid(const uint8_t op)
{
	return op < op__count;
}

// check opcode for branching
inline bool isBranch(const Opcode op)
{
	return op == op_br || op == op_cbr;
}

// machine word -- 31-bit, plus a hidden (non-architectural) bit
struct Word {
	uint32_t word : 31;
	uint32_t reserved : 1;

	// construct word from architectural and non-architectural parts
	constexpr Word(uint32_t word, uint32_t reserved) : word(word), reserved(reserved) {}
	// construct architectural word from uint32_t -- implicit
	Word(uint32_t word) : word(word), reserved(0) {}
	// convert architectural word to uint32_t
	operator uint32_t () const { return word; }

	// pre-increment architectural word (for address arithmetics)
	Word& operator ++() { ++word; return *this; }
	// post-increment architectural word (for address arithmetics)
	Word operator ++(int) { const Word res = *this; ++word; return res; }
	// pre-decrement architectural word (for address arithmetics)
	Word& operator --() { --word; return *this; }
	// post-decrement architectural word (for address arithmetics)
	Word operator --(int) { const Word res = *this; --word; return res; }
};

constexpr Word word_invalid{0, 1};
constexpr Word word_min_int{(1U << 30), 0}; // two's complement: -(2^30)
constexpr Word word_max_int{(1U << 30) - 1, 0}; // two's complement: 2^30 - 1

inline bool isWordValid(const Word word)
{
	return 0 == word.reserved;
}

// machine instruction
// Up to 3 operands r0..r2, in dense order; 1-operand instructions use r0; 2-operand instructions use r0..r1; when present, dst operand is r0.
// For opcode 'li' r1..r2 contain a little-endian immediate value, i.e. r1: LSB, r2: MSB. Unused operands contain reg-invalid.
class __attribute__ ((aligned(4))) Instr {
	constexpr static size_t MAX_OPERAND_COUNT = 3; // non-negotiable symbolic constant
	Operand r[MAX_OPERAND_COUNT]; // instruction operands, 1st through last (reg-invalid for operands past the last)
	Opcode op; // instruction opcode; most-significant bit reserved

public:
	explicit Instr(const Opcode op) : op(op) {}
	// get instruction opcode
	Opcode getOpcode() const;
	// get instruction operand at specified index
	Operand getOperand(const size_t index) const;
	// set instruction operand at specified index; optionally invalidate the remaining operands
	void setOperand(const size_t index, const Operand reg, const bool invalidateRest = false);
	// get instruction immediate operand
	uint32_t getImm() const;
};

inline Opcode Instr::getOpcode() const
{
	switch (op) {
	case op_nop:
		if (reg_invalid == r[0] && reg_invalid == r[1] && reg_invalid == r[2])
			return op;
		break;
	case op_li:
		if (reg_invalid != r[0])
			return op;
		break;
	case op_push:
	case op_pop:
		if (reg_invalid != r[0] && reg_invalid == r[1] && reg_invalid == r[2])
			return op;
		break;
	case op_br:
		if (reg_invalid != r[0] && reg_invalid == r[1] && reg_invalid == r[2])
			return op;
		break;
	case op_cbr:
		if (reg_invalid != r[0] && reg_invalid != r[1] && reg_invalid != r[2])
			return op;
		break;
	case op_op2:
		if (reg_invalid != r[0] && reg_invalid != r[1] && reg_invalid == r[2])
			return op;
		break;
	case op_op3:
		if (reg_invalid != r[0] && reg_invalid != r[1] && reg_invalid != r[2])
			return op;
		break;
	}
	return op_invalid;
}

inline Operand Instr::getOperand(const size_t index) const
{
	assert(index < MAX_OPERAND_COUNT);
	return r[index];
}

inline void Instr::setOperand(const size_t index, const Operand reg, const bool invalidateRest)
{
	assert(index < MAX_OPERAND_COUNT);
	r[index] = reg;

	if (invalidateRest) {
		for (size_t i = index + 1; i < MAX_OPERAND_COUNT; ++i)
			r[i] = reg_invalid;
	}
}

inline uint32_t Instr::getImm() const
{
	assert(op_li == op);
	return Word(int16_t(uint16_t(r[1]) + (uint16_t(r[2]) << 8)), 0);
}

inline const char* strFromOpcode(const Opcode op)
{
	switch (op) {
	case op_nop:
		return "nop";
	case op_li:
		return "li";
	case op_push:
		return "push";
	case op_pop:
		return "pop";
	case op_br:
		return "br";
	case op_cbr:
		return "cbr";
	case op_op2:
	case op_op3:
		return "op";
	}

	return "invalid";
}

extern "C" {
	void string_x16(void*, uint32_t);
	void string_x32(void*, uint32_t);
}

static size_t strFromInstr(const Instr& instr, char* buffer, const size_t bufferSize)
{
	const Opcode op = instr.getOpcode();
	const char* const strop = strFromOpcode(op);

	const size_t stroplen = strlen(strop);
	size_t minBufferSize = stroplen;
	size_t noperand = 0;

	// determine operand count and textual footprint
	switch (op) {
	case op_li:
		minBufferSize += strlen("\t0000, 0x00000000");
		noperand = 1;
		break;
	case op_push:
	case op_pop:
	case op_br:
		minBufferSize += strlen("\t0000");
		noperand = 1;
		break;
	case op_cbr:
		minBufferSize += strlen("\t0000, 0000, 0000");
		noperand = 3;
		break;
	case op_op2:
		minBufferSize += strlen("\t0000, 0000");
		noperand = 2;
		break;
	case op_op3:
		minBufferSize += strlen("\t0000, 0000, 0000");
		noperand = 3;
		break;
	}

	if (buffer && bufferSize > minBufferSize) {
		memcpy(buffer, strop, stroplen);
		size_t pos = stroplen;

		if (noperand) {
			buffer[pos++] = '\t';
			string_x16(buffer + pos, instr.getOperand(0));
			pos += 4;

			for (size_t i = 1; i < noperand; ++i) {
				buffer[pos++] = ',';
				buffer[pos++] = ' ';
				string_x16(buffer + pos, instr.getOperand(i));
				pos += 4;
			}

			if (op_li == op) {
				buffer[pos++] = ',';
				buffer[pos++] = ' ';
				buffer[pos++] = '0';
				buffer[pos++] = 'x';
				string_x32(buffer + pos, instr.getImm());
				pos += 8;
			}
		}
		// terminate
		assert(pos == minBufferSize);
		buffer[pos] = '\0';
	}

	return minBufferSize + 1; // account for terminator
}

} // namespace isa

#endif // __isa_h
