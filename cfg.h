#if !defined(__cfg_h)
#define __cfg_h

#include <assert.h>
#include <utility>
#include <map>
#include "bb.h"

// Control-flow graph -- nodes constitute basic blocks, edges -- branches to a basic-block start addresses

namespace cfg {

class ControlFlowGraph : std::map< bb::Address, bb::BasicBlock > { // TODO: change map for something more appropriate
	map& basecast() { return *static_cast< map* >(this); }
	const map& basecast() const { return *static_cast< const map* >(this); }

public:
	bool addBasicBlock(bb::BasicBlock&&);
	bb::BasicBlock* getBasicBlock(const bb::Address);
	const bb::BasicBlock* getBasicBlock(const bb::Address) const;

	using map::const_iterator;
	const_iterator begin() const;
	const_iterator end() const;
};

inline bool ControlFlowGraph::addBasicBlock(bb::BasicBlock&& bb)
{
	using namespace bb;
	const Address bbAddress = bb.getStartAddress();
	assert(isAddressValid(bbAddress));

	struct Interval {
		Address begin;
		Address end;

		bool overlap(const Interval& oth) const {
			return begin < oth.end && oth.begin < end;
		}
	};

	const Interval incoming = { .begin = bbAddress, .end = bbAddress + Address(bb.getSequence().size()) };
	const std::pair< const_iterator, const_iterator > range = equal_range(bbAddress);

	// check preceding elemets for address overlaps
	for (const_reverse_iterator it = std::make_reverse_iterator<const_iterator>(range.first); it != rend(); ++it) {
		const Address presentAddr = it->second.getStartAddress();
		const Interval present = { .begin = presentAddr, .end = presentAddr + Address(it->second.getSequence().size()) };

		if (incoming.begin >= present.end)
			break;

		if (incoming.overlap(present))
			return false;
	}

	// check succeeding elements for address overlaps
	for (const_iterator it = range.second; it != end(); ++it) {
		const Address presentAddr = it->second.getStartAddress();
		const Interval present = { .begin = presentAddr, .end = presentAddr + Address(it->second.getSequence().size()) };

		if (present.begin >= incoming.end)
			break;

		if (present.overlap(incoming))
			return false;
	}

	insert(value_type(bb.getStartAddress(), std::move(bb)));
	return true;
}

inline bb::BasicBlock* ControlFlowGraph::getBasicBlock(const bb::Address start)
{
	const iterator it = find(start);
	return it != basecast().end() ? &it->second : nullptr;
}

inline const bb::BasicBlock* ControlFlowGraph::getBasicBlock(const bb::Address start) const
{
	const const_iterator it = find(start);
	return it != basecast().end() ? &it->second : nullptr;
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
