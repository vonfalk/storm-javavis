#pragma once
#include "Core/GcArray.h"
#include "Core/EnginePtr.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Buffer for use with streams. Designed to make it possible to allocate on the stack to avoid
	 * heap allocations (at least from C++).
	 *
	 * If multiple copies of a buffer is created, they all refer to the same backing storage,
	 * slightly breaking the memory model of Storm.
	 *
	 * TODO: Make proper element access operators from Storm.
	 */
	class Buffer {
		STORM_VALUE;
	public:
		// Default constructor. Empty buffer.
		STORM_CTOR Buffer();

		// Copy a buffer.
		Buffer(const Buffer &src);

		// Number of bytes in total.
		inline Nat STORM_FN count() const { return data ? data->count : 0; }

		// Number of bytes containing valid data. Constrains 'filled' to not be larger than 'count'.
		inline Nat STORM_FN filled() const { return data ? data->filled : 0; }
		inline void STORM_FN filled(Nat p) { if (data) data->filled = min(p, count()); }

		// Element access.
		Byte &operator [](Nat id) { return data->v[id]; }
		Byte operator [](Nat id) const { return data->v[id]; }

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Get a pointer to the data.
		byte *dataPtr() { return data ? data->v : null; }

		// Empty/full?
		inline Bool STORM_FN empty() const { return filled() == 0; }
		inline Bool STORM_FN full() const { return filled() == count(); }

	private:
		// Data.
		GcArray<byte> *data;

		// From C++: create a buffer with a pre-allocated array.
		Buffer(GcArray<byte> *data);

		friend Buffer buffer(EnginePtr e, Nat count);
		friend Buffer emptyBuffer(GcArray<byte> *data);
		friend Buffer fullBuffer(GcArray<byte> *data);
		friend Buffer buffer(EnginePtr e, const byte *data, Nat count);
	};

	// Create a buffer.
	Buffer STORM_FN buffer(EnginePtr e, Nat count);

	// Create a buffer using a (potentially stack-allocated) GcArray for backing data. This function
	// assumes that the backing array contains data, so it sets 'filled' to the size of the
	// data. Use the constructor if this is not desired.
	Buffer emptyBuffer(GcArray<byte> *data);
	Buffer fullBuffer(GcArray<byte> *data);

	// Create a buffer by copying data from 'data'. Sets 'filled'.
	Buffer buffer(EnginePtr e, const byte *data, Nat count);

	// Grow a buffer.
	Buffer STORM_FN grow(EnginePtr e, Buffer src, Nat newCount);

	// Get a 'substring' of a buffer.
	Buffer STORM_FN cut(EnginePtr e, Buffer src, Nat from);
	Buffer STORM_FN cut(EnginePtr e, Buffer src, Nat from, Nat count);

	// Output.
	void STORM_FN outputMark(StrBuf &to, Buffer b, Nat markAt);
	StrBuf &STORM_FN operator <<(StrBuf &to, Buffer b);

}
