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
	void addBasicBlock(bb::BasicBlock&&);
	bb::BasicBlock* getBasicBlock(const bb::Address);
	const bb::BasicBlock* getBasicBlock(const bb::Address) const;

	using map::const_iterator;
	const_iterator begin() const;
	const_iterator end() const;
};

inline void ControlFlowGraph::addBasicBlock(bb::BasicBlock&& bb)
{
	assert(bb::isAddressValid(bb.getStartAddress()));
	assert(find(bb.getStartAddress()) == end());
	insert(value_type(bb.getStartAddress(), std::move(bb)));
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
