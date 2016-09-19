#pragma once
#include "Core/TObject.h"
#include "Core/Array.h"
#include "Size.h"
#include "Label.h"
#include "Reference.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Binary code output interface. Always outputs in word-order suitable for the current
	 * architecture.
	 */
	class Output : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		virtual void STORM_FN putByte(Byte b);  // 1 byte
		virtual void STORM_FN putInt(Nat w);    // 4 bytes
		virtual void STORM_FN putLong(Word w);  // 8 bytes
		virtual void STORM_FN putPtr(Word w);   // 4 or 8 bytes

		/**
		 * Special cases for GC interaction.
		 */

		// Put a pointer to a gc:d object. 4 or 8 bytes.
		virtual void STORM_FN putGcPtr(Word w);

		// Put a relative pointer to a gc:d object. 4 or 8 bytes. (Note: this may be tricky on x86-64).
		virtual void STORM_FN putGcRelative(Word w);

		// Put a relative pointer to a static object (not managed by the Gc). 4 or 8 bytes.
		virtual void STORM_FN putRelativeStatic(Word w);

		// Put an absolute pointer somwhere inside ourself. 4 or 8 bytes.
		virtual void STORM_FN putPtrSelf(Word w);


		// Get the current offset from start.
		virtual Nat STORM_FN tell() const;

		// Write a word with a given size.
		void STORM_FN putSize(Word w, Size size);

		// Labels.
		void STORM_FN mark(Label lbl);
		void STORM_FN putRelative(Label lbl); // Writes 4 bytes.
		void STORM_FN putAddress(Label lbl); // Writes 4 or 8 bytes.

		// References.
		void STORM_FN putRelative(Ref ref); // Writes 4 or 8 bytes.
		void STORM_FN putAddress(Ref ref); // Writes 4 or 8 bytes.

	protected:
		// Mark a label here.
		virtual void STORM_FN markLabel(Nat id);

		// Mark the last added gc-pointer as a reference to something.
		virtual void STORM_FN markGcRef(Ref ref);

		// Get a pointer to the start of the code.
		virtual void *codePtr() const;

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

		// Total # of bytes needed. Also readable through 'tell'.
		Nat size;

		// # of references needed
		Nat refs;

		virtual void STORM_FN putByte(Byte b);
		virtual void STORM_FN putInt(Nat w);
		virtual void STORM_FN putLong(Word w);
		virtual void STORM_FN putPtr(Word w);
		virtual void STORM_FN putGcPtr(Word w);
		virtual void STORM_FN putGcRelative(Word w);
		virtual void STORM_FN putRelativeStatic(Word w);
		virtual void STORM_FN putPtrSelf(Word w);

		virtual Nat STORM_FN tell() const;

	protected:
		virtual void STORM_FN markLabel(Nat id);
		virtual void *codePtr() const;
		virtual Nat STORM_FN labelOffset(Nat id);
		virtual Nat STORM_FN toRelative(Nat offset);

	private:
		Nat ptrSize;
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

	/**
	 * Reference which will update a reference in a code segment. Make sure to keep these alive as
	 * long as the code segement is alive somehow.
	 */
	class CodeUpdater : public Reference {
		STORM_CLASS;
	public:
		CodeUpdater(Ref src, Content *inside, void *code, Nat slot);

		// Notification of a new location.
		virtual void moved(const void *newAddr);

	private:
		// The code segment to update.
		UNKNOWN(PTR_GC) void *code;

		// Which slot to update.
		Nat slot;
	};

}
