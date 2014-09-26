#pragma once

#include "Instruction.h"
#include "Label.h"
#include "Frame.h"

#include <vector>
#include <map>
#include "Utils/HashMap.h"

namespace code {

	// Represents a code listing, can be linked into machine code.
	class Listing : public util::Printable {
	public:
		Listing();

		// Create a new listing based on "other", the new listing is created
		// so that any labels found in "other" will also be acceptable in the created one.
		static Listing createShell(const Listing &other);

		// Create a label.
		Label label();

		// Get the label reserved for metadata (used by backends).
		Label metadata();

		// The stack frame associated with this listing.
		Frame frame;

		// Get the current number of labels.
		inline nat getLabelCount() const { return nextLabelId; }
		// Get label id in front of instruction "i" (line->label)
		std::multimap<nat, Label> getLabels() const;
		//std::multimap<nat, nat> getLabels() const;

		// Append an instruction.
		Listing &operator <<(const Instruction &instr);
		// Add a label to the next instruction.
		Listing &operator <<(const Label &label);
		// Append all instructions in another listing. Does _not_ copy any labels or scope information.
		Listing &operator <<(const Listing &listing);

		// Access to instructions (no modifications yet).
		inline const Instruction &operator[] (nat id) const { return code[id]; }
		inline nat size() const { return code.size(); }

	protected:
		void output(std::wostream &to) const;

	private:
		nat nextLabelId;

		// The instructions that makes up the listing.
		vector<Instruction> code;

		// Label index to instruction id.
		typedef hash_map<nat, nat> Labels;
		Labels labels;
	};

	// Transform callbacks.
	class Transformer {
	public:
		inline Transformer(const Listing &from) : from(from) {}

		const Listing &from;

		// Only use once!
		Listing transformed();

	protected:
		virtual void before(Listing &to);
		virtual void transform(Listing &to, nat line) = 0;
		virtual void after(Listing &to);
	};

}