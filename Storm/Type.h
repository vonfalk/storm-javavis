#pragma once
#include "Named.h"

namespace storm {

	/**
	 * Define different properties for a type.
	 */
	enum TypeFlags {
		// Regular type.
		typeClass = 0x00,

		// Is it a value type (does not inherit from Object).
		typeValue = 0x01,
	};

	inline TypeFlags operator &(TypeFlags a, TypeFlags b) { return TypeFlags(nat(a) & nat(b)); }
	inline TypeFlags operator |(TypeFlags a, TypeFlags b) { return TypeFlags(nat(a) | nat(b)); }

	/**
	 * Represents a type. This class contains information about
	 * all type members, fields and so on.
	 */
	class Type : public Named {
	public:
		Type(const String &name, TypeFlags flags);
		~Type();

		// Type name.
		const String name;

		// Type flags.
		const TypeFlags flags;

		// Get the size of this type.
		virtual nat size() const;

	protected:
		virtual void output(wostream &to) const;

	private:
		// Member functions.

		// Super type (when inheritance is used).
		Type *superType;
	};

}
