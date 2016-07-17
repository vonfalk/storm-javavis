#pragma once
#include "OS/Thread.h"
#include "OS/Threadgroup.h"

namespace storm {

	/**
	 * Common types in Storm:
	 */

	typedef bool Bool; // 8-bit
	typedef code::Byte Byte; // 8-bit, unsigned
	typedef code::Int Int; // 32-bit, signed
	typedef code::Nat Nat; // 32-bit, unsigned
	typedef code::Long Long; // 64-bit, signed
	typedef code::Word Word; // 64-bit, unsigned
	typedef code::Float Float; // 32-bit float

	class Engine;
	class Object;
	class Type;
	class CloneEnv;
	class Str;
	class StrBuf;
	class Thread;

	using code::Size;
	using code::Offset;

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

}
