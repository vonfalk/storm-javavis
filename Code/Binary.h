#pragma once

#include "Arena.h"
#include "Listing.h"
#include "MachineCode.h"
#include "RefSource.h"

namespace code {

	/**
	 * This class represents a listing that has been translated into
	 * machine code along with any extra information specified by the
	 * Listing object, such as descriptions of the stack for portions
	 * of the generated code and so on. The Binary object is therefore
	 * created from a listing and remains static afterwards.
	 *
	 * The title is used as the title of the RefSource object.
	 */
	class Binary : public Content {
	public:
		Binary(Arena &arena, const Listing &listing);
		~Binary();

		// Update a refsource with our data.
		// Consider creating a new Binary instead, as this can lead to trouble in multi-threaded environments.
		void update(const Listing &listing);

		// Find the active block, given a specific location in the generated code.
		Block blockAt(void *ip);

		// Read various variable types, given a stack-frame's base pointer as defined by the backend.
		Byte readByte(machine::StackFrame &frame, Variable v);
		Int readInt(machine::StackFrame &frame, Variable v);
		Long readLong(machine::StackFrame &frame, Variable v);
		void *readPtr(machine::StackFrame &frame, Variable v);

		// Destroy variables on the current stack frame.
		void destroyFrame(const machine::StackFrame &frame) const;

		// Arena
		Arena &arena;

		// Dump handlers.
		void dbg_dump();

	private:
		// The address of the backend's code metadata region.
		machine::FnMeta *metadata;

		// Keep track of all references that might need to be updated.
		vector<Reference *> references;

		// Information about a single variable.
		struct Var {
			// Variable id.
			nat id;

			// Variable size.
			Size size;
		};

		// Store information about a single part. Note that this looks like a tree!
		struct Part {
			// Parent scope if any, otherwise == this block id.
			nat parent;

			// Number of variables in this scope.
			nat variables;

			// All variables in this scope.
			Var variable[1];

			// Create a block.
			static Part *create(const Frame &frame, const code::Part &b);
		};

		// All parts in this code. The backend keeps track of which is the currently active part.
		vector<Part *> parts;

		// Update the contents of this binary. Bonus: overrides set() in RefSource for us!
		void set(const Listing &listing);

		void updateReferences(void *addr, const Output &from);
		void updateParts(const Frame &from);

		// Destroy a single variable.
		void destroyVariable(const machine::StackFrame &frame, Var var) const;
	};

	// Reference updater for addresses, used within the Binary class itself.
	class BinaryUpdater : public Reference {
	public:
		BinaryUpdater(const Ref &ref, const Content &owner, cpuNat *at, bool relative);

		virtual void onAddressChanged(void *newAddress);
	private:
		cpuNat *at;
		bool relative;
	};
}
