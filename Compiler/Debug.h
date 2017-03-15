#pragma once
#include "Core/Object.h"
#include "Core/EnginePtr.h"
#include "Thread.h"
#include "Utils/Bitmask.h"

namespace storm {
	namespace debug {
		STORM_PKG(core.debug);

		/**
		 * Various types used for debugging.
		 */


		/**
		 * Simple linked-list.
		 */
		class Link : public Object {
			STORM_CLASS;
		public:
			nat value;
			Link *next;
		};

		// Create and check linked lists containing the elements 0..length-1
		MAYBE(Link *) STORM_FN createList(EnginePtr e, Nat length);
		Bool STORM_FN checkList(MAYBE(Link *) start, Nat length);


		/**
		 * Value-type containing pointers.
		 */
		class Value {
			STORM_VALUE;
		public:
			nat value;
			Link *list;
		};


		/**
		 * Class containing value-types.
		 */
		class ValClass : public Object {
			STORM_CLASS;
		public:
			Value data;
			ValClass *next;
		};


		/**
		 * Simple class with finalizer.
		 *
		 * The destructor prints 'value'.
		 */
		class DtorClass : public Object {
			STORM_CLASS;
		public:
			DtorClass(int v);
			~DtorClass();
			int value;
		};


		/**
		 * Class using pointer-equality in maps.
		 */
		class PtrKey : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			PtrKey();

			// Have we moved recently?
			bool moved() const;

			// Reset moved.
			void reset();

		private:
			size_t oldPos;
		};


		/**
		 * Class with a member we can easily extend.
		 */
		class Extend : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Extend();
			STORM_CTOR Extend(Int v);

			virtual Int STORM_FN value();

		private:
			Int v;
		};



		/**
		 * Debug functions useful while developing the compiler itself. May or may
		 * not be available in release mode later on. TODO: decide.
		 */

		void STORM_FN dbgBreak();

		// Return a float from C++.
		Float STORM_FN dbgFloat();

		// Crude printing (not final version).
		void STORM_FN print(Object *obj);
		void STORM_FN print(TObject *obj);

		// Print the stack (machine specific, for debug).
		void STORM_FN dumpStack();

		// Throw an exception.
		void STORM_FN throwError();

		// Check so that 'a' and 'b' are completely disjoint.
		Bool STORM_FN disjoint(Object *a, Object *b);

		// Check so that 'a' and 'b' are the same object.
		Bool STORM_FN same(Object *a, Object *b);

		// Output the layout of 'a'.
		void STORM_FN layout(Object *a);

		// Very crude sleep implementation, it pauses all UThreads that happens to be running on the
		// OS thread of this thread. This has to be improved later on!
		void STORM_FN dbgSleep(Int ms);

		// Yeild from the currenlty running UThread. How should this be implemented later?
		void STORM_FN dbgYeild();

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

			void STORM_FN deepCopy(CloneEnv *env);

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
			static os::Lock liveLock;
		};

		Str *STORM_FN toS(EnginePtr e, DbgVal v);


		// Class partly implemented in C++, we'll try to override this in Storm.
		// NOTE: We are _not_ overriding toS here to see if overriding functions from
		// classes other than the direct parent is working.
		class Dbg : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Dbg();
			Dbg(Dbg *o);

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
			virtual void STORM_FN deepCopy(CloneEnv *env);

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
			Str *STORM_FN echo(Str *str);

		private:
			Int v;
		};


		// Class without a ToS function.
		class DbgNoToS {
			STORM_VALUE;
		public:
			STORM_CTOR DbgNoToS();
			Int dummy;
		};


		// Enum.
		enum DbgEnum {
			foo,
			bar,
		};

		// Bitmask enum
		enum DbgBit {
			bitFoo = 0x01,
			bitBar = 0x02,
		};

		BITMASK_OPERATORS(DbgBit);
	}
}
