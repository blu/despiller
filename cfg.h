#if !defined(__cfg_h)
#define __cfg_h

#include <assert.h>
#include <utility>
#include <vector>
#include <set>
#include "bb.h"
#include "reg.h"

// Control-flow graph -- nodes constitute basic blocks, edges -- branches to a basic-block start addresses

namespace cfg {

enum RegOrder {
	REG_ENTRY, // registry at entry
	REG_EXIT   // registry at exit
};

struct LessBB {
	bool operator ()(const bb::BasicBlock& lhs, const bb::BasicBlock& rhs) const {
		using namespace bb;
		const Address laddr = lhs.getStartAddress();
		const Address raddr = rhs.getStartAddress();
		return laddr < raddr;
	}
};

class ControlFlowGraph {
	struct BBAndReg : bb::BasicBlock
	{
		BBAndReg(const bb::BasicBlock& src) : bb::BasicBlock(src) {}
		BBAndReg(bb::BasicBlock&& src) : bb::BasicBlock(std::move(src)) {}

		reg::Registry reg[2]; // at-entry, at-exit
	};
	typedef std::set< BBAndReg, LessBB > BBlocks;

	BBlocks bblocks; // basic-block nodes in the CFG

	typedef std::vector< reg::Value > Values;
	typedef std::vector< Values > Stack;

	Stack stack; // stack for 'storage'

public:
	// add basic block to the CFG
	bool addBasicBlock(bb::BasicBlock&&);
	// look up basic block in the CFG, mutable version
	bb::BasicBlock* getBasicBlock(const bb::Address);
	// look up basic block in the CFG, immutable version
	const bb::BasicBlock* getBasicBlock(const bb::Address) const;

	// set registry at BB entry in the CFG; mandates a pre-existing BB
	bool setRegistry(const bb::Address, reg::Registry&&);
	// compute registry at BB exit in the CFG; mandates a pre-existing BB
	bool calcRegistry(const bb::Address);
	// look up registry in the CFG, mutable version
	reg::Registry* getRegistry(const bb::Address, const RegOrder);
	// look up registry in the CFG, immutable version
	const reg::Registry* getRegistry(const bb::Address, const RegOrder) const;

	typedef BBlocks::const_iterator const_iterator;
	// get immutable start iterator of the CFG (first element)
	const_iterator begin() const;
	// get immutable end iterator of the CFG (one past the final element)
	const_iterator end() const;

	// clear the stack storage
	void stackClear();
};

inline bool ControlFlowGraph::addBasicBlock(bb::BasicBlock&& bb)
{
	using namespace bb;
	const Address bbAddress = bb.getStartAddress();

	struct Interval {
		Address begin;
		Address end;

		bool overlap(const Interval& oth) const {
			return begin < oth.end && oth.begin < end;
		}
	};

	const Interval incoming = { .begin = bbAddress, .end = bbAddress + Address(bb.getSequence().size()) };
	const std::pair< const_iterator, const_iterator > range = bblocks.equal_range(bb);

	// check preceding elemets for address overlaps
	for (BBlocks::const_reverse_iterator it = std::make_reverse_iterator<const_iterator>(range.first); it != bblocks.rend(); ++it) {
		const Address presentAddr = it->getStartAddress();
		const Interval present = { .begin = presentAddr, .end = presentAddr + Address(it->getSequence().size()) };

		if (incoming.begin >= present.end)
			break;

		if (incoming.overlap(present))
			return false;
	}

	// check succeeding elements for address overlaps
	for (BBlocks::const_iterator it = range.second; it != bblocks.end(); ++it) {
		const Address presentAddr = it->getStartAddress();
		const Interval present = { .begin = presentAddr, .end = presentAddr + Address(it->getSequence().size()) };

		if (present.begin >= incoming.end)
			break;

		if (present.overlap(incoming))
			return false;
	}

	bblocks.insert(std::move(bb));
	return true;
}

inline bb::BasicBlock* ControlFlowGraph::getBasicBlock(const bb::Address start)
{
	// following const_cast may look like trouble but the so-obtained BB actually
	// cannot be mutated to a dregree where it could violate the container order
	const BBlocks::iterator it = bblocks.find(bb::BasicBlock(start));
	return it != bblocks.end() ? const_cast< BBAndReg* >(&*it) : nullptr;
}

inline const bb::BasicBlock* ControlFlowGraph::getBasicBlock(const bb::Address start) const
{
	const BBlocks::const_iterator it = bblocks.find(bb::BasicBlock(start));
	return it != bblocks.end() ? &*it : nullptr;
}

inline bool ControlFlowGraph::setRegistry(const bb::Address bbAddress, reg::Registry&& src)
{
	BBAndReg* const p = static_cast< BBAndReg* >(getBasicBlock(bbAddress));

	if (!p)
		return false;

	p->reg[REG_ENTRY] = std::move(src);
	return true;
}

inline bool ControlFlowGraph::calcRegistry(const bb::Address bbAddress)
{
	BBAndReg* const p = static_cast< BBAndReg* >(getBasicBlock(bbAddress));

	if (!p)
		return false;

	using namespace bb;
	using namespace isa;

	Address currAddress = bbAddress;
	reg::Registry currReg = p->reg[0];

	const Instructions& seq = p->getSequence();
	for (const auto it : seq) {
		Operand args[] = {
			reg_invalid,
			reg_invalid,
			reg_invalid
		};
		const Opcode op = it.getOpcode();
		// get dst register
		switch (op) {
		case op_li:
		case op_push:
		case op_pop:
		case op_br:
		case op_cbr:
		case op_op2:
		case op_op3:
			args[0] = it.getOperand(0);
			break;
		}
		// get src0 register
		switch (op) {
		case op_cbr:
		case op_op2:
		case op_op3:
			args[1] = it.getOperand(1);
			break;
		}
		// get src1 register
		switch (op) {
		case op_cbr:
		case op_op3:
			args[2] = it.getOperand(2);
			break;
		}
		// verify operand0 validity if branch or push op
		if (reg_invalid != args[0] && (isBranch(op) || op_push == op) && !currReg.occupied(args[0])) {
			fprintf(stderr, "error: instr at %p references an unoccupied 1st-operand register %04x\n", currAddress, args[0]);
			return false;
		}
		// verify operand1 validity if applicable
		if (reg_invalid != args[1] && !currReg.occupied(args[1])) {
			fprintf(stderr, "error: instr at %p references an unoccupied 2nd-operand register %04x\n", currAddress, args[1]);
			return false;
		}
		// verify operand2 validity if applicable
		if (reg_invalid != args[2] && !currReg.occupied(args[2])) {
			fprintf(stderr, "error: instr at %p references an unoccupied 3rd-operand register %04x\n", currAddress, args[2]);
			return false;
		}
		// update current registry according to op
		Values values;
		switch (op) {
		case op_li:
			currReg.addValue(it.getOperand(0), it.getImm());
			break;
		case op_push:
			for (const auto iv : currReg.getValues(it.getOperand(0)))
				values.push_back(iv.second);
			currReg.vacate(it.getOperand(0));
			stack.push_back(std::move(values));
			break;
		case op_pop:
			assert(!stack.empty());
			currReg.vacate(it.getOperand(0));
			for (const auto iv : stack.back())
				currReg.addValue(it.getOperand(0), iv);
			stack.pop_back();
			break;
		}
		++currAddress;
	}

	p->reg[REG_EXIT] = std::move(currReg);
	return true;
}

inline reg::Registry* ControlFlowGraph::getRegistry(const bb::Address start, const RegOrder order)
{
	// following const_cast may look like trouble but the so-obtained BB actually
	// cannot be mutated to a dregree where it could violate the container order
	const BBlocks::iterator it = bblocks.find(bb::BasicBlock(start));
	return it != bblocks.end() ? const_cast< reg::Registry* >(&it->reg[order]) : nullptr;
}

inline const reg::Registry* ControlFlowGraph::getRegistry(const bb::Address start, const RegOrder order) const
{
	const BBlocks::const_iterator it = bblocks.find(bb::BasicBlock(start));
	return it != bblocks.end() ? &it->reg[order] : nullptr;
}

inline ControlFlowGraph::const_iterator ControlFlowGraph::begin() const
{
	return bblocks.begin();
}

inline ControlFlowGraph::const_iterator ControlFlowGraph::end() const
{
	return bblocks.end();
}

inline void ControlFlowGraph::stackClear()
{
	stack.clear();
}

} // namespace cfg

#endif // __cfg_h
