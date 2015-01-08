#pragma once
#include "Overload.h"
#include "Value.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A variable contained in a type. This is used to allocate
	 * space in the host object.
	 */
	class TypeVar : public NameOverload {
		STORM_CLASS;
	public:
		TypeVar(Type *owner, const Value &type, const String &name);

		// Our type.
		const Value varType;

		// Find here (never returns anything useful).
		virtual Named *find(const Name &name);

	protected:
		// Output.
		virtual void output(wostream &to) const;
	};

}
