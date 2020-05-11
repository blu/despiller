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

class Registry {
public:
	typedef std::multimap< Register, Value > Values;

private:
	Values values;

public:
	// add unknown to the given register; at most one unknown tracked per register
	void addUnknown(const Register);
	// add new value to the given register
	void addValue(const Register, const Value);
	// vacate the given register -- remove all records of it
	void vacate(const Register);

	// get all known values for the given register
	std::pair< Values::const_iterator, Values::const_iterator > getValues(const Register) const;
	// get occupancy of the given register, whether by values or unknowns
	bool occupied(const Register) const;

	// add the content of another registry to the registry
	void merge(const Registry&);
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

inline std::pair< Registry::Values::const_iterator, Registry::Values::const_iterator > Registry::getValues(const Register reg) const
{
	const std::pair< Values::const_iterator, Values::const_iterator > range = values.equal_range(reg);
	return range;
}

inline bool Registry::occupied(const Register reg) const
{
	return values.find(reg) != values.end();
}

} // namespace reg

#endif // __reg_h
