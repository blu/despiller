#if !defined(__cfg_h)
#define __cfg_h

#include <assert.h>
#include <utility>
#include <set>
#include "bb.h"
#include "reg.h"

// Control-flow graph -- nodes constitute basic blocks, edges -- branches to a basic-block start addresses

namespace cfg {

struct LessBB {
	bool operator ()(const bb::BasicBlock& lhs, const bb::BasicBlock& rhs) const {
		using namespace bb;
		const Address laddr = lhs.getStartAddress();
		const Address raddr = rhs.getStartAddress();
		return laddr < raddr;
	}
};

typedef std::pair< reg::Registry, reg::Registry > RegistryPair;

class ControlFlowGraph {
	typedef std::set< bb::BasicBlock, LessBB > BBlocks;
	typedef std::map< bb::Address, RegistryPair > Registries;

	BBlocks bblocks; // basic-block nodes in the CFG
	Registries registries; // a pair of registries for each BB: first -- entry, second -- exit

public:
	// add basic block to the CFG
	bool addBasicBlock(bb::BasicBlock&&);
	// look up basic block in the CFG, mutable version
	bb::BasicBlock* getBasicBlock(const bb::Address);
	// look up basic block in the CFG, immutable version
	const bb::BasicBlock* getBasicBlock(const bb::Address) const;
	// look up registry pair in the CFG, mutable version
	RegistryPair* getRegistry(const bb::Address);
	// look up registry pair in the CFG, immutable version
	const RegistryPair* getRegistry(const bb::Address) const;


	typedef BBlocks::const_iterator const_iterator;
	// get immutable start iterator of the CFG (lowest start address)
	const_iterator begin() const;
	// get immutable end iterator of the CFG (one past the final element)
	const_iterator end() const;
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
	return it != bblocks.end() ? const_cast< bb::BasicBlock* >(&*it) : nullptr;
}

inline const bb::BasicBlock* ControlFlowGraph::getBasicBlock(const bb::Address start) const
{
	const BBlocks::const_iterator it = bblocks.find(bb::BasicBlock(start));
	return it != bblocks.end() ? &*it : nullptr;
}

inline RegistryPair* ControlFlowGraph::getRegistry(const bb::Address start)
{
	const Registries::iterator it = registries.find(start);
	return it != registries.end() ? &it->second : nullptr;
}

inline const RegistryPair* ControlFlowGraph::getRegistry(const bb::Address start) const
{
	const Registries::const_iterator it = registries.find(start);
	return it != registries.end() ? &it->second : nullptr;
}

inline ControlFlowGraph::const_iterator ControlFlowGraph::begin() const
{
	return bblocks.begin();
}

inline ControlFlowGraph::const_iterator ControlFlowGraph::end() const
{
	return bblocks.end();
}

} // namespace cfg

#endif // __cfg_h
