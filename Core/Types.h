#pragma once
#include "OS/Types.h"
#include "OS/Thread.h"
#include "OS/Threadgroup.h"

namespace storm {

	/**
	 * Common types in Storm:
	 */

	typedef bool Bool; // 8-bit
	typedef os::Byte Byte; // 8-bit, unsigned
	typedef os::Int Int; // 32-bit, signed
	typedef os::Nat Nat; // 32-bit, unsigned
	typedef os::Long Long; // 64-bit, signed
	typedef os::Word Word; // 64-bit, unsigned
	typedef os::Float Float; // 32-bit float

	class Engine;
	class Object;
	class Type;
	class CloneEnv;
	class Str;
	class StrBuf;
	class Thread;
	class Handle;

	/**
	 * Class used when declaring named threads from C++.
	 */
	struct DeclThread {
		// Our unique id.
		nat identifier;

		// Function used to create the Thread (if any).
		typedef os::Thread (*CreateFn)(Engine &);
		CreateFn createFn;

		// Get the thread we are representing.
		Thread *thread(Engine &e) const;

		// TODO: implement 'threadName' and 'force' here.
	};

	/**
	 * Struct used to get access to private members of other classes.
	 */
	struct CppMeta;

	STORM_PKG(core);

	// The maybe type. This will let the preprocessor recognize Maybe<T>, but C++ will disallow
	// it. Use MAYBE(T) instead.
	STORM_TEMPLATE(Maybe, createMaybe);
}
