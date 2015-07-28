#pragma once
#include "Object.h"
#include "Shared/TObject.h"
#include "Int.h"
#include "OS/Sync.h"

namespace storm {
	STORM_PKG(core.debug);

	/**
	 * Debug functions useful while developing the compiler itself. May or may
	 * not be available in release mode later on. TODO: decide.
	 */

	void STORM_FN dbgBreak();

	// Return a float from C++.
	Float STORM_FN dbgFloat();

	// Print the VTable of an object.
	void STORM_FN printVTable(Object *obj);
	void STORM_FN printVTable(TObject *obj);

	// Crude printing (not final version).
	void STORM_FN print(Object *obj);
	void STORM_FN print(TObject *obj);

	// Dump info on an object.
	void STORM_FN printInfo(Object *obj);

	// Basic print tracing (to be removed when we have something real).
	void STORM_FN ptrace(Int z);

	// Print the stack (machine specific, for debug).
	void STORM_FN dumpStack();

	// Throw an exception.
	void STORM_FN throwError();

	// Check so that 'a' and 'b' are completely disjoint.
	Bool STORM_FN disjoint(Par<Object> a, Par<Object> b);

	// Check so that 'a' and 'b' are the same object.
	Bool STORM_FN same(Par<Object> a, Par<Object> b);

	// Output the layout of 'a'.
	void STORM_FN layout(Par<Object> a);

	// Very crude sleep implementation, it pauses all UThreads that happens to be running on the
	// OS thread of this thread. This has to be improved later on!
	void STORM_FN dbgSleep(Int ms);

	// Yeild from the currenlty running UThread. How should this be implemented later?
	void STORM_FN dbgYeild();

	// Get the type of an object (debug).
	Type *STORM_FN dbgTypeOf(Object *o);
	Type *STORM_FN dbgTypeOf(TObject *o);

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

		void STORM_FN deepCopy(Par<CloneEnv> env);

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

		// Lock for the live set.
		static Lock liveLock;
	};

	// Class partly implemented in C++, we'll try to override this in Storm.
	// NOTE: We are _not_ overriding toS here to see if overriding functions from
	// classes other than the direct parent is working.
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

		// Convert it to a DbgVal!
		virtual DbgVal STORM_FN asDbgVal();

		// For use in VTableTest.
		virtual Int STORM_FN returnOne();
		virtual Int STORM_FN returnTwo();

		// Deep copy.
		virtual void STORM_FN deepCopy(Par<CloneEnv> env);

	private:
		Int v;
	};


	wostream &operator <<(wostream &to, const DbgVal &v);

	// Object on another thread.
	class DbgActor : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		STORM_CTOR DbgActor();
		STORM_CTOR DbgActor(Int z);

		virtual void STORM_FN set(Int v);
		virtual Int STORM_FN get() const;

		// To dbgVal
		virtual DbgVal STORM_FN asDbgVal();

		// Echo a string (to verify that copies are being made).
		Str *STORM_FN echo(Par<Str> str);

	private:
		Int v;
	};

}
