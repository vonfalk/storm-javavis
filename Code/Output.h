#pragma once
#include "Core/TObject.h"
#include "Core/Array.h"
#include "Core/GcCode.h"
#include "Reg.h"
#include "Size.h"
#include "Label.h"
#include "Reference.h"
#include "OffsetReference.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Binary code output interface. Always outputs in word-order suitable for the current
	 * architecture.
	 */
	class Output : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		/**
		 * Low-level output.
		 */

		virtual void STORM_FN putByte(Byte b);  // 1 byte
		virtual void STORM_FN putInt(Nat w);    // 4 bytes
		virtual void STORM_FN putLong(Word w);  // 8 bytes
		virtual void STORM_FN putPtr(Word w);   // 4 or 8 bytes

		// Align the output pointer for the next 'put' operation.
		virtual void STORM_FN align(Nat to);

		/**
		 * Special cases for GC interaction.
		 */

		// Put a custom purpose GC pointer. 'size' bytes.
		virtual void putGc(GcCodeRef::Kind kind, Nat size, Word w);
		void putGc(GcCodeRef::Kind kind, Nat size, Ref ref);

		// Put a pointer to a gc:d object. 4 or 8 bytes.
		virtual void STORM_FN putGcPtr(Word w);

		// Put a relative pointer to a gc:d object. 4 or 8 bytes.
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
		void STORM_FN mark(MAYBE(Array<Label> *) lbl);
		void STORM_FN putRelative(Label lbl); // Writes 4 bytes.
		void STORM_FN putRelative(Label lbl, Nat offset);
		void STORM_FN putOffset(Label lbl); // Writes 4 bytes. Offset relative to the start of the blob.
		void STORM_FN putAddress(Label lbl); // Writes 4 or 8 bytes.

		// References.
		void STORM_FN putRelative(Ref ref); // Writes 4 or 8 bytes.
		void STORM_FN putAddress(Ref ref); // Writes 4 or 8 bytes.
		void putObject(RootObject *obj); // Writes 4 or 8 bytes.

		void STORM_FN putInt(OffsetRef ref); // Writes 4 bytes.
		void STORM_FN putPtr(OffsetRef ref); // Writes 4 or 8 bytes.

		// Store a (4-byte) reference to a reference or to an object.
		void STORM_FN putObjRelative(Ref ref);
		void putObjRelative(RootObject *obj);

		/**
		 * Call frame information: used during stack unwinding.
		 */

		// Mark the end of the prolog.
		virtual void STORM_FN markProlog();
		// Mark the end of the epilog.
		virtual void STORM_FN markEpilog();
		// Mark that a register has been saved at 'offset' relative ptrFrame.
		virtual void STORM_FN markSaved(Reg reg, Offset offset);

	protected:
		// Mark a label here.
		virtual void STORM_FN markLabel(Nat id);

		// Mark the last added gc-pointer as a reference to something.
		virtual void STORM_FN markGcRef(Ref ref);

		// Mark the last added int as a reference 'ref' that needs to be kept up to date.
		virtual void STORM_FN markRef(OffsetRef ref, Bool ptr);

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
		virtual void STORM_FN align(Nat to);
		virtual void putGc(GcCodeRef::Kind kind, Nat size, Word w);
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

	/**
	 * Reference which will update an offset-reference in a code segment.
	 */
	class CodeOffsetUpdater : public OffsetReference {
		STORM_CLASS;
	public:
		CodeOffsetUpdater(OffsetRef src, Content *inside, void *code, Nat codeOffset, Bool ptr);

		// Notification of a new location.
		virtual void moved(Offset newAddr);

	private:
		// The code segment to update.
		UNKNOWN(PTR_GC) void *code;

		// Offset inside 'code' to update.
		Nat codeOffset;

		// Pointer-sized?
		Bool ptr;
	};

}
