#pragma once
#include "Value.h"
#include "Named.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * A variable contained in a type. This is used to allocate
	 * space in the host object.
	 */
	class TypeVar : public Named {
		STORM_CLASS;
	public:
		TypeVar(Type *owner, const Value &type, const String &name);

		// Owning type.
		Type *owner() const;

		// Get our offset.
		Offset offset() const;

		// Our type.
		const Value varType;

		// Find here (never returns anything useful).
		virtual Named *find(const Name &name);

	protected:
		// Output.
		virtual void output(wostream &to) const;
	};

}
