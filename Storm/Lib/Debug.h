#pragma once
#include "Object.h"
#include "Int.h"

namespace storm {
	STORM_PKG(core.debug);

	/**
	 * Debug functions useful while developing the compiler itself. May or may
	 * not be available in release mode later on. TODO: decide.
	 */

	void STORM_FN dbgBreak();

	// Print the VTable of an object.
	void STORM_FN printVTable(Object *obj);

	// Crude printing (not final version).
	void STORM_FN print(Object *obj);

	// Basic print tracing (to be removed when we have something real).
	void STORM_FN ptrace(Int z);

	// Print the stack (machine specific, for debug).
	void STORM_FN dumpStack();

	// Throw an exception.
	void STORM_FN throwError();

	// Check so that 'a' and 'b' are completely disjoint.
	Bool STORM_FN disjoint(Par<Object> a, Par<Object> b);

	// Output the layout of 'a'.
	void STORM_FN layout(Par<Object> a);

	// Class partly implemented in C++, we'll try to override this in Storm.
	class Dbg : public Object {
		STORM_CLASS;
	public:
		STORM_CTOR Dbg();
		STORM_CTOR Dbg(Par<Dbg> o);

		// Set the value as well...
		STORM_CTOR Dbg(Int v);

		// Dtor.
		~Dbg();

		// Print our state.
		void STORM_FN dbg();

		// Set a value.
		void STORM_FN set(Int v);

		// Get a value.
		virtual Int STORM_FN get();

		// For use in VTableTest.
		virtual Int STORM_FN returnOne();
		virtual Int STORM_FN returnTwo();

	private:
		Int v;
	};


	// Value implemented in C++ for testing. Tracks live instances!
	class DbgVal {
		STORM_VALUE;
	public:
		STORM_CTOR DbgVal();
		STORM_CTOR DbgVal(Int v);

		~DbgVal();
		DbgVal(const DbgVal &o);
		DbgVal &operator =(const DbgVal &o);

		bool operator ==(const DbgVal &o) const;

		Int v;

		// Manipulations.
		void STORM_FN set(Int v);
		Int STORM_FN get() const;

		// Instance checks.

		// Returns 'true' if all instances were cleared before.
		static bool clear();

		// Prints all live objects.
		static void dbg_dump();

		// Current live instances.
		typedef std::set<const DbgVal*> LiveSet;
		static LiveSet live;
	};

	wostream &operator <<(wostream &to, const DbgVal &v);

}
