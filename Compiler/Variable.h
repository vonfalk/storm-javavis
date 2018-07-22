#pragma once
#include "Named.h"
#include "NamedThread.h"
#include "RunOn.h"
#include "Core/Fn.h"

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


	/**
	 * A global variable stored in the name tree. Global variables are only supposed to be
	 * accessible from their associated thread, so that proper synchronization is enforced by the
	 * compiler. Consider two global variables that form a single state together. Disallowing access
	 * from other threads than the "owning" thread forces the programmer to write accessor functions
	 * on the proper thread, which ensures that any updates to the state happen as intended. Having
	 * multiple threads accessing the global variables could cause unintended race conditions.
	 *
	 * TODO: Expose 'dataPtr' to Storm in a good way. Perhaps by exposing Engine::ref so that ASM
	 * can access it at least.
	 */
	class GlobalVar : public Variable {
		STORM_CLASS;
	public:
		// Create. Initialize the variable to whatever is returned by 'initializer'. 'initializer'
		// is evaluated on the thread specified by 'thread'.
		STORM_CTOR GlobalVar(Str *name, Value type, NamedThread *thread, FnBase *initializer);

		// Owning thread.
		NamedThread *owner;

		// Assuming we're running on 'thread', may we access this variable?
		Bool STORM_FN accessibleFrom(RunOn thread);

		// Get the pointer to the data. Safe to call from any thread.
		void *CODECALL dataPtr();

		// Get a string representation of the value. Returns null if no string representation is available.
		Str *STORM_FN strValue() const;

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// The data stored in the variable. If the variable is an Object or a TObject, this is just
		// a pointer to the object itself. If it is a value type, this is a pointer to an array of
		// size 1 which contains the object.
		UNKNOWN(PTR_GC) void *data;

		// Do we have an array for this type?
		Bool hasArray;
	};

}
