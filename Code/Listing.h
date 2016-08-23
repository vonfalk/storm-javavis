#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Instr.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * A label inside a listing.
	 */
	class Label {
		STORM_VALUE;
	public:
		inline Bool STORM_FN operator ==(Label o) const { return id == o.id; }
		inline Bool STORM_FN operator !=(Label o) const { return id != o.id; }

	private:
		friend class Listing;
		friend wostream &operator <<(wostream &to, Label l);
		friend StrBuf &operator <<(StrBuf &to, Label l);

		explicit Label(Nat id);

		Nat id;
	};

	wostream &operator <<(wostream &to, Label l);
	StrBuf &operator <<(StrBuf &to, Label l);


	/**
	 * Represents a code listing along with information about blocks and variables. Can be linked
	 * into machine code.
	 */
	class Listing : public Object {
		STORM_CLASS;
	public:
		// Create an empty listing.
		STORM_CTOR Listing();

		// Copy.
		STORM_CTOR Listing(const Listing &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		/**
		 * One entry in the listing:
		 */
		class Entry {
			STORM_VALUE;
		public:
			STORM_CTOR Entry();
			STORM_CTOR Entry(const Entry &o);
			STORM_CTOR Entry(Instr *instr);
			STORM_CTOR Entry(Engine &e, Label label);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// The instruction at this point.
			MAYBE(Instr *) instr;

			// Labels before this instruction (if any). Only created if any labels are added.
			MAYBE(Array<Label> *) labels;

			// Add a label here.
			void add(Engine &e, Label label);
		};

		// Add instructions and labels.
		Listing &operator <<(Instr *op);
		Listing &operator <<(Label l);

		// Label reserved for metadata.
		Label STORM_FN meta();

		// Create a new label.
		Label STORM_FN label();

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Instructions and labels in here.
		Array<Entry> *code;

		// Last used label id.
		Nat lastLabel;
	};

	StrBuf &STORM_FN operator <<(StrBuf &to, Listing::Entry e);

}
