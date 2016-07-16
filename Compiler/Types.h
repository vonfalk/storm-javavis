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
		// Function used to create the Thread (if any).
		// The address of this member is our unique identifier.
		typedef os::Thread (*CreateFn)(Engine &);
		CreateFn createFn;

		// Get the thread we are representing.
		Thread *thread(Engine &e) const;

		// Get our unique identifier.
		inline uintptr_t identifier() const {
			return (uintptr_t)&createFn;
		}

		// TODO: implement 'threadName' and 'force' here.
	};

}
