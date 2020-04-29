#if !defined(__isa_h)
#define __isa_h

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

// Fake ISA whose sole purpose is to demonstrate the effect of de-spilling

// This ISA has an unspecified-size GPR file R0..Rn, but it also has an unlimited storage space 'storage'
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

typedef uint8_t Register;

constexpr Opcode op_invalid = static_cast< Opcode >(uint8_t(-1));
constexpr Register reg_invalid = Register(-1);

class Instr {
	constexpr static size_t MAX_OPERAND_COUNT = 3;
	Opcode op; // instruction opcode
	Register r[MAX_OPERAND_COUNT]; // instruction operands, 1st through last (reg-invalid for operands past the last)
	// For opcode 'li', r[0] contains the destination, whereas r[1..2] contain an immediate value, little endian

public:
	Instr(const Opcode op) : op(op) {}
	Opcode getOp() const;
	Register getOperand(const size_t index) const;
	void setOperand(const size_t index, const Register reg, const bool invalidateRest = false);
};

inline Opcode Instr::getOp() const
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

inline Register Instr::getOperand(const size_t index) const
{
	assert(index < MAX_OPERAND_COUNT);
	return r[index];
}

void Instr::setOperand(const size_t index, Register reg, const bool invalidateRest)
{
	assert(index < MAX_OPERAND_COUNT);
	r[index] = reg;

	if (invalidateRest) {
		for (size_t i = index + 1; i < MAX_OPERAND_COUNT; ++i)
			r[i] = reg_invalid;
	}
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
	const Opcode op = instr.getOp();
	const char* const strop = strFromOpcode(op);

	constexpr unsigned regLimit = 0x10000;
	const size_t stroplen = strlen(strop);
	size_t minBufferSize = stroplen;
	size_t noperand = 0;

	// determine operand count and textual footprint
	switch (op) {
	case op_li:
		assert(regLimit > instr.getOperand(0));
		minBufferSize += strlen("\t0000, 0x0000");
		noperand = 1;
		break;
	case op_push:
	case op_pop:
	case op_br:
		assert(regLimit > instr.getOperand(0));
		minBufferSize += strlen("\t0000");
		noperand = 1;
		break;
	case op_cbr:
		assert(regLimit > instr.getOperand(0) && regLimit > instr.getOperand(1) && regLimit > instr.getOperand(2));
		minBufferSize += strlen("\t0000, 0000, 0000");
		noperand = 3;
		break;
	case op_op2:
		assert(regLimit > instr.getOperand(0) && regLimit > instr.getOperand(1));
		minBufferSize += strlen("\t0000, 0000");
		noperand = 2;
		break;
	case op_op3:
		assert(regLimit > instr.getOperand(0) && regLimit > instr.getOperand(1) && regLimit > instr.getOperand(2));
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
		}
		if (op_li == op) {
			const uint32_t imm = uint32_t(instr.getOperand(1)) + (uint32_t(instr.getOperand(2)) << 8);
			buffer[pos++] = ',';
			buffer[pos++] = ' ';
			buffer[pos++] = '0';
			buffer[pos++] = 'x';
			string_x16(buffer + pos, imm);
			pos += 4;
		}
		// terminate
		assert(pos == minBufferSize);
		buffer[pos] = '\0';
	}

	return minBufferSize + 1; // account for terminator
}

} // namespace isa

#endif // __isa_h
