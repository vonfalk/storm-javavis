#include "stdafx.h"
#include "Output.h"

namespace code {

	void Output::putByte(Byte b) {
		assert(false);
	}

	void Output::putInt(Nat w) {
		assert(false);
	}

	void Output::putLong(Word w) {
		assert(false);
	}

	void Output::putPtr(Word w) {
		assert(false);
	}

	Nat Output::tell() const {
		assert(false);
		return 0;
	}

	void Output::putSize(Word w, Size size) {
		if (size == Size::sByte)
			putByte(Byte(w));
		else if (size == Size::sInt)
			putInt(Nat(w));
		else if (size == Size::sLong)
			putLong(w);
		else if (size == Size::sPtr)
			putPtr(w);
		else
			assert(false, L"Unknown size passed to putSize!");
	}

	void Output::mark(Label lbl) {
		markLabel(lbl.id);
	}

	void Output::putRelative(Label lbl) {
		putInt(toRelative(labelOffset(lbl.id)));
	}

	void Output::markLabel(Nat id) {
		assert(false);
	}

	void Output::putAddress(Label lbl) {
		TODO(L"Implement!");
	}

	Nat Output::labelOffset(Nat id) {
		assert(false);
		return 0;
	}

	Nat Output::toRelative(Nat offset) {
		assert(false);
		return 0;
	}


	LabelOutput::LabelOutput(Nat ptrSize) : offsets(new (engine()) Array<Nat>()), ptrSize(ptrSize), size(0) {}

	void LabelOutput::putByte(Byte b) {
		size += 1;
	}

	void LabelOutput::putInt(Nat w) {
		size += 4;
	}

	void LabelOutput::putLong(Word w) {
		size += 8;
	}

	void LabelOutput::putPtr(Word w) {
		size += ptrSize;
	}

	Nat LabelOutput::tell() const {
		return size;
	}

	void LabelOutput::markLabel(Nat id) {
		while (offsets->count() <= id)
			offsets->push(0);

		offsets->at(id) = tell();
	}

	Nat LabelOutput::labelOffset(Nat id) {
		if (id < offsets->count())
			return offsets->at(id);
		else
			return 0;
	}

	Nat LabelOutput::toRelative(Nat offset) {
		// Irrelevant here.
		return 0;
	}


	CodeOutput::CodeOutput() {}

	void *CodeOutput::codePtr() const {
		return null;
	}

}
