#if !defined(__cfg_h)
#define __cfg_h

#include <assert.h>
#include <utility>
#include <set>
#include "bb.h"

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

class ControlFlowGraph : std::set< bb::BasicBlock, LessBB > {
	set& basecast() { return *static_cast< set* >(this); }
	const set& basecast() const { return *static_cast< const set* >(this); }

public:
	bool addBasicBlock(bb::BasicBlock&&);
	bb::BasicBlock* getBasicBlock(const bb::Address);
	const bb::BasicBlock* getBasicBlock(const bb::Address) const;

	using set::const_iterator;
	const_iterator begin() const;
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
	const std::pair< const_iterator, const_iterator > range = equal_range(bb);

	// check preceding elemets for address overlaps
	for (const_reverse_iterator it = std::make_reverse_iterator<const_iterator>(range.first); it != rend(); ++it) {
		const Address presentAddr = it->getStartAddress();
		const Interval present = { .begin = presentAddr, .end = presentAddr + Address(it->getSequence().size()) };

		if (incoming.begin >= present.end)
			break;

		if (incoming.overlap(present))
			return false;
	}

	// check succeeding elements for address overlaps
	for (const_iterator it = range.second; it != end(); ++it) {
		const Address presentAddr = it->getStartAddress();
		const Interval present = { .begin = presentAddr, .end = presentAddr + Address(it->getSequence().size()) };

		if (present.begin >= incoming.end)
			break;

		if (present.overlap(incoming))
			return false;
	}

	insert(std::move(bb));
	return true;
}

inline bb::BasicBlock* ControlFlowGraph::getBasicBlock(const bb::Address start)
{
	const iterator it = find(bb::BasicBlock(start));
	return it != basecast().end() ? const_cast< bb::BasicBlock* >(&*it) : nullptr;
}

inline const bb::BasicBlock* ControlFlowGraph::getBasicBlock(const bb::Address start) const
{
	const const_iterator it = find(bb::BasicBlock(start));
	return it != basecast().end() ? &*it : nullptr;
}

inline ControlFlowGraph::const_iterator ControlFlowGraph::begin() const
{
	return basecast().begin();
}

inline ControlFlowGraph::const_iterator ControlFlowGraph::end() const
{
	return basecast().end();
}

} // namespace cfg

#endif // __cfg_h
