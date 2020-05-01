#if !defined(__bb_h)
#define __bb_h

#include <stdint.h>
#include <vector>
#include "isa.h"

// Basic block -- a block of instructions executed sequentially

namespace bb {

// address in a Von-Neumann machine -- word granularity
typedef uint32_t Address;

// sequence of instructions
typedef std::vector< isa::Instr > Instructions;

constexpr Address addr_invalid = Address(-1);
constexpr Address addr_topbit = Address(-1) << sizeof(Address) * 8 - 1;

inline bool isAddressValid(Address addr)
{
	return 0 == (addr & addr_topbit);
}

inline Address invalidateAddress(const Address addr)
{
	return addr | addr_topbit;
}

inline Address revalidateAddress(const Address addr)
{
	return addr & ~addr_topbit;
}

class BasicBlock {
	Address start; // basic-block start address
	Address exit[2]; // two potential branch targets for exit from the basic block (two when the BB ends with a conditional branch)
	Instructions instr; // basic-block instructions

public:
	explicit BasicBlock(const Address start) : start(start), exit{ addr_invalid, addr_invalid } {}
	BasicBlock(BasicBlock&&);
	// get start address of the basic block
	Address getStartAddress() const;
	// get one of the branch targets at the exit of the basic block
	Address getExitTargetAddress(const size_t index) const;
	// get the immutable instruction sequence of the basic block
	const Instructions& getSequence() const;
	// append instruction to basic block
	void addInstr(const isa::Instr&);
	// replace existing instruction in the basic block
	void replaceInstr(const size_t index, const isa::Instr newInstr);
	// check the validity of the basic block
	bool validate();
};

inline BasicBlock::BasicBlock(BasicBlock&& src)
: start(src.start)
, instr(std::move(src.instr))
{}

inline Address BasicBlock::getStartAddress() const
{
	return start;
}

inline Address BasicBlock::getExitTargetAddress(const size_t index) const
{
	assert(index < sizeof(exit) / sizeof(exit[0]));
	return exit[index];
}

inline const Instructions& BasicBlock::getSequence() const
{
	return instr;
}

inline bool isBranch(const isa::Opcode op)
{
	using namespace isa;

	return op == op_br || op == op_cbr;
}

inline void BasicBlock::addInstr(const isa::Instr& newInstr)
{
	instr.push_back(newInstr);
}

inline void BasicBlock::replaceInstr(const size_t index, const isa::Instr newInstr)
{
	assert(index < instr.size());
	instr[index] = newInstr;
}

// check the validity of the basic block; invalid BBs are:
// a) empty
// b) containing an invalid op
// c) containing an early branch
inline bool BasicBlock::validate()
{
	using namespace isa;

	Opcode op = op_invalid;
	Instructions::const_iterator it = instr.begin();
	const Instructions::const_iterator itend = instr.end();
	for (; it != itend; ++it) {
		op = it->getOpcode();
		if (!isOpcodeValid(op) || isBranch(op))
			break;
	}

	if (!isOpcodeValid(op) || it != itend && ++it != itend) {
		start = invalidateAddress(start);
		return false;
	}

	// unless ending with an unconditional branch, one of the branch
	// targets out of this BB is the first address immediately after
	// this BB; any other targets will be resolved at linking
	exit[0] = op_br != instr.back().getOpcode() ? start + instr.size() : addr_invalid;
	exit[1] = addr_invalid;

	return true;
}

} // namespace bb

#endif // __bb_h
