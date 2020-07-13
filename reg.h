#if !defined(__reg_h)
#define __reg_h

#include <stdint.h>
#include <assert.h>
#include <map>
#include "isa.h"

// GPR file occupancy map -- stores constants and unknowns, per register

namespace reg {

typedef isa::Operand Register;
typedef isa::Word Value;
typedef std::multimap< Register, Value > Values;

// next type needs to be a derivation and not a mere typedef -- latter breaks ranged-for
struct ValueRange : std::pair< Values::const_iterator, Values::const_iterator >
{
	ValueRange(const std::pair< Values::const_iterator, Values::const_iterator >& src) : pair(src) {}
	ValueRange(std::pair< Values::const_iterator, Values::const_iterator >&& src) : pair(std::move(src)) {}
};

inline Values::const_iterator begin(const ValueRange& range)
{
	return range.first;
}

inline Values::const_iterator end(const ValueRange& range)
{
	return range.second;
}

class Registry {
	Values values;

public:
	// add unknown to the given register; at most one unknown tracked per register
	void addUnknown(const Register);
	// add new value to the given register
	void addValue(const Register, const Value);
	// vacate the given register -- remove all records of it
	void vacate(const Register);

	// get all known values for the given register
	ValueRange getValues(const Register) const;
	// get occupancy of the given register, whether by values or unknowns
	bool occupied(const Register) const;

	// add the content of another registry to this one
	void merge(const Registry&);

	// get immutable start iterator of the registry (first element)
	Values::const_iterator begin() const;
	// get immutable end iterator of the registry (one past the final element)
	Values::const_iterator end() const;
};

inline void Registry::addValue(const Register reg, const Value val)
{
	// check if this reg-val pair is already present
	const std::pair< Values::const_iterator, Values::const_iterator > range = values.equal_range(reg);
	for (Values::const_iterator it = range.first; it != range.second; ++it) {
		if (it->second == val)
			return;
	}

	values.insert(Values::value_type(reg, val));
}

inline void Registry::addUnknown(const Register reg)
{
	addValue(reg, isa::word_invalid);
}

inline void Registry::vacate(const Register reg)
{
	// erase any values
	const std::pair< Values::const_iterator, Values::const_iterator > range = values.equal_range(reg);
	values.erase(range.first, range.second);
}

inline ValueRange Registry::getValues(const Register reg) const
{
	const std::pair< Values::const_iterator, Values::const_iterator > range = values.equal_range(reg);
	return range;
}

inline bool Registry::occupied(const Register reg) const
{
	return values.find(reg) != values.end();
}

inline void Registry::merge(const Registry& oth)
{
	for (auto it : oth.values)
		addValue(it.first, it.second);
}

inline Values::const_iterator Registry::begin() const
{
	return values.begin();
}

inline Values::const_iterator Registry::end() const
{
	return values.end();
}

} // namespace reg

#endif // __reg_h
