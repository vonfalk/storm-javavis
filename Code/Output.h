#pragma once

#include "Reference.h"
#include "Label.h"

namespace code {

	// Binary code-output interface. Always outputs in word-order
	// suitable for the current architecture.
	class Output : NoCopy {
	public:
		virtual void putByte(Byte b) = 0; // 1 byte
		virtual void putInt(Nat w) = 0; // 4 bytes
		virtual void putLong(Word w) = 0; // 8 bytes
		virtual void putPtr(cpuNat v) = 0; // 4 or 8 bytes

		// Write a word with a given size (0 = ptr)
		// TODO? Replace 'nat' with 'Size'
		void putSize(Word w, nat size);

		void putRelative(cpuNat addr);

		void putAddress(const Ref &ref);
		void putRelative(const Ref &ref);

		// Mark the current position as label "id".
		void markLabel(Label lbl);

		void putAddress(const Label &lbl);
		void putRelative(const Label &lbl);

		// Get the current offset from the start.
		virtual nat tell() const = 0;

		// A list of all used absolute references and relative references.
		// These are populated with putAddress(Ref) and putRelative(Ref).
		struct UsedRef {
			Ref reference;
			nat offset;
		};
		vector<UsedRef> absoluteRefs;
		vector<UsedRef> relativeRefs;

	protected:
		virtual cpuNat lookupLabel(nat id) = 0;
		virtual cpuNat toRelative(cpuNat addr) = 0;
		virtual void markLabelId(nat id) = 0;

		static inline nat labelId(Label lbl) { return lbl.id; }
	};

	// Specialization for size-computation and label offset generation. Does not write anything to memory.
	class SizeOutput : public Output {
	public:
		SizeOutput(nat numLabels);

		// Value used to represent an uninitialized label offset.
		static const cpuNat invalidOffset = -1;

		// List of all offsets.
		vector<cpuNat> labelOffsets;

		virtual void putByte(Byte b);
		virtual void putInt(Nat w);
		virtual void putLong(Word w);
		virtual void putPtr(cpuNat v);

		virtual nat tell() const;

	protected:
		virtual cpuNat lookupLabel(nat id);
		virtual cpuNat toRelative(cpuNat addr);
		virtual void markLabelId(nat id);

	private:
		nat size;
	};

	// Specialization for code output to memory.
	class MemoryOutput : public Output {
	public:
		// Note: keeps a reference to labelOffsets in "metrics"!
		MemoryOutput(void *to, SizeOutput &metrics);

		virtual void putByte(Byte b);
		virtual void putInt(Nat w);
		virtual void putLong(Word w);
		virtual void putPtr(cpuNat v);

		virtual nat tell() const;

		// Compute the address of 'lbl'. Returns null if the label has not been defined yet.
		virtual void *lookup(Label lbl);
	protected:
		virtual cpuNat lookupLabel(nat id);
		virtual cpuNat toRelative(cpuNat addr);
		virtual void markLabelId(nat id);

	private:
		const vector<cpuNat> &labelOffsets;

		byte *dest;
		nat size;
		nat at;
	};

}
