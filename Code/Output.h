#pragma once
#include "Core/TObject.h"
#include "Core/Array.h"
#include "Size.h"
#include "Label.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Binary code output interface. Always outputs in word-order suitable for the current
	 * architecture.
	 */
	class Output : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		virtual void STORM_FN putByte(Byte b); // 1 byte
		virtual void STORM_FN putInt(Nat w);   // 4 bytes
		virtual void STORM_FN putLong(Word w); // 8 bytes
		virtual void STORM_FN putPtr(Word w);  // 4 or 8 bytes

		// Get the current offset from start.
		virtual Nat STORM_FN tell() const;

		// Write a word with a given size.
		void STORM_FN putSize(Word w, Size size);

		// Labels. Note: only relative label offsets are supported!
		void STORM_FN mark(Label lbl);
		void STORM_FN putRelative(Label lbl); // Writes 4 bytes.

		// Output absolute addresses (maybe not supported in the end).
		void STORM_FN putAddress(Label lbl); // Writes 8 bytes.

	protected:
		// Mark a label here.
		virtual void STORM_FN markLabel(Nat id);

		// Find the offset of a label.
		virtual Nat STORM_FN labelOffset(Nat id);

		// Convert an absolute offset to a relative offset.
		virtual Nat STORM_FN toRelative(Nat offset);
	};

	/**
	 * Output variant producing positions for all labels, and the overall size needed by the output.
	 */
	class LabelOutput : public Output {
		STORM_CLASS;
	public:
		STORM_CTOR LabelOutput(Nat ptrSize);

		// Store all label offsets here.
		Array<Nat> *offsets;

		virtual void STORM_FN putByte(Byte b);
		virtual void STORM_FN putInt(Nat w);
		virtual void STORM_FN putLong(Word w);
		virtual void STORM_FN putPtr(Word w);
		virtual Nat STORM_FN tell() const;

	protected:
		virtual void STORM_FN markLabel(Nat id);
		virtual Nat STORM_FN labelOffset(Nat id);
		virtual Nat STORM_FN toRelative(Nat offset);

	private:
		Nat ptrSize;
		Nat size;
	};

	/**
	 * Code output. Provides a pointer to code in the end.
	 */
	class CodeOutput : public Output {
		STORM_CLASS;
	public:
		STORM_CTOR CodeOutput();

		virtual void *codePtr() const;
	};

}
