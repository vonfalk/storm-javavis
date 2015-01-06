#pragma once
#include "Size.h"

namespace code {

	// Represents a variable in a block. This has information about
	// its size and its current stack offset. The offset from the stack
	// is not static among modifications to any blocks in the listing.
	class Variable {
		friend class Frame;
		friend class Value;
	public:
		// Initialize to nothing.
		Variable();

		static const Variable invalid;

		// Get the variable's ID, mainly for backends.
		inline nat getId() const { return id; }

		inline Size size() const { return sz; }

		// Note, small public api since this class is mainly owned by the Frame object.

		inline bool operator ==(const Variable &o) const { return id == o.id; }
		inline bool operator !=(const Variable &o) const { return id != o.id; }
	private:
		Variable(nat id, Size sz);

		// Unique identifier, provided by the block.
		nat id;

		// Size of the variable
		Size sz;
	};

	// Represents a block of code in a listing. These blocks contain a set of
	// local variables, along with parent-child relations. This information is
	// available in the final Binary-object as well. The binary object will then
	// provide information of the current active block(s) in relation to an
	// execution point in the program.
	class Block {
		friend class Frame;
		friend class Value;
	public:
		// Initialize to nothing.
		Block();

		static const Block invalid;

		// Get the block's ID, mainly for backends.
		inline nat getId() const { return id; }

		// No public interface, everything is handled by the owning Listing.

		inline bool operator ==(const Block &o) const { return id == o.id; }
		inline bool operator !=(const Block &o) const { return id != o.id; }
	private:
		Block(nat id);

		// Unique identified, provided by the listing.
		nat id;
	};


}
