#include "stdafx.h"
#include "Listing.h"
#include "Core/StrBuf.h"

namespace code {

	Label::Label(Nat id) : id(id) {}

	wostream &operator <<(wostream &to, Label l) {
		return to << l.id;
	}

	StrBuf &operator <<(StrBuf &to, Label l) {
		return to << l.id;
	}

	Listing::Entry::Entry() : instr(null), labels(null) {}

	Listing::Entry::Entry(const Entry &o) {
		// It is immutable anyway.
		instr = o.instr;
		// Deep copy the array.
		labels = o.labels ? new (o.labels) Array<Label>(o.labels) : null;
	}

	Listing::Entry::Entry(Instr *i) : instr(i), labels(null) {}

	Listing::Entry::Entry(Engine &e, Label l) : instr(null) {
		labels = new (e) Array<Label>(l);
	}

	void Listing::Entry::deepCopy(CloneEnv *env) {
		// We do everything in the copy ctor.
	}

	void Listing::Entry::add(Engine &e, Label l) {
		if (!labels)
			labels = new (e) Array<Label>(l);
		else
			labels->push(l);
	}

	/**
	 * Listing.
	 */

	Listing::Listing() : code(new (engine()) Array<Entry>()), lastLabel(0) {}

	Listing::Listing(const Listing &o) : code(new (engine()) Array<Entry>(o.code)), lastLabel(o.lastLabel) {}

	void Listing::deepCopy(CloneEnv *env) {
		// Everything is done in the copy-ctor.
	}

	Listing &Listing::operator <<(Instr *i) {
		if (code->empty()) {
			*code << Entry(i);
		} else if (code->last().instr) {
			*code << Entry(i);
		} else {
			code->last().instr = i;
		}
		return *this;
	}

	Listing &Listing::operator <<(Label l) {
		if (code->empty()) {
			*code << Entry(engine(), l);
		} else if (code->last().instr) {
			*code << Entry(engine(), l);
		} else {
			code->last().add(engine(), l);
		}
		return *this;
	}

	Label Listing::meta() {
		return Label(0);
	}

	Label Listing::label() {
		return Label(++lastLabel);
	}

	void Listing::toS(StrBuf *to) const {
		// TODO: Output the frame.

		for (nat i = 0; i < code->count(); i++) {
			*to << L"\n" << code->at(i);
		}
	}

	StrBuf &STORM_FN operator <<(StrBuf &to, Listing::Entry e) {
		if (e.labels != null && e.labels->any()) {
			StrBuf *tmp = new (&to) StrBuf();
			*tmp << e.labels->at(0);

			for (nat i = 1; i < e.labels->count(); i++)
				*tmp << L", " << width(3) << e.labels->at(i);

			*tmp << L": ";

			to << width(20) << tmp->toS();
		} else {
			to << width(20) << L" ";
		}

		if (e.instr)
			to << e.instr;

		return to;
	}

}
