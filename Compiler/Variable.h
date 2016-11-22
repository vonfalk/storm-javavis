#pragma once
#include "Named.h"

namespace storm {
	STORM_PKG(core.lang);

	/**
	 * Describes some kind of variable. It is a member variable if it takes a parameter, otherwise
	 * it is some kind of global variable.
	 */
	class Variable : public Named {
		STORM_CLASS;
	public:
		// Our type.
		Value type;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	protected:
		// Create a variable, either as a member or as a non-member.
		STORM_CTOR Variable(Str *name, Value type);
		STORM_CTOR Variable(Str *name, Value type, Type *member);
	};


	/**
	 * Represents a member-variable to some kind of type. The variable is uniquely identified by its
	 * offset relative to the start of the object.
	 */
	class MemberVar : public Variable {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR MemberVar(Str *name, Value type, Type *memberOf);

		// Get the offset of this member.
		Offset STORM_FN offset() const;

		// Get our parent type.
		Type *owner() const;

		// Set our offset.
		void setOffset(Offset off);

		// TODO: We might want to offer references for using the offset, so we can easily update it.
	private:
		// The actual offset. Updated by 'layout'.
		Offset off;

		// Has the layout been produced?
		Bool hasLayout;
	};

}