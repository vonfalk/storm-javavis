#pragma once

#include "Arena.h"
#include "Listing.h"
#include "MachineCode.h"

namespace code {

	/**
	 * This class represents a listing that has been translated into
	 * machine code along with any extra information specified by the
	 * Listing object, such as descriptions of the stack for portions
	 * of the generated code and so on. The Binary object is therefore
	 * created from a listing and remains static afterwards.
	 *
	 * The title is used to identify who is referring a specific object, and
	 * should therefore be a human-readable string.
	 */
	class Binary : NoCopy {
	public:
		Binary(Arena &arena, const String &title, const Listing &listing);
		~Binary();

		// Update a refsource with our data.
		void update(RefSource &ref);

		// Get the current pointer to the underlying binary data.
		const void *getData() const;

		// Find the active block, given a specific location in the generated code.
		Block blockAt(void *ip);

		// Read various variable types, given a stack-frame's base pointer as defined by the backend.
		Byte readByte(machine::StackFrame &frame, Variable v);
		Int readInt(machine::StackFrame &frame, Variable v);
		Long readLong(machine::StackFrame &frame, Variable v);
		void *readPtr(machine::StackFrame &frame, Variable v);

		// Destroy variables on the current stack frame.
		void destroyFrame(const machine::StackFrame &frame) const;
	private:
		// Title.
		String title;

		// Arena
		Arena &arena;

		// Allocated code chunk
		void *memory;
		nat size;

		// The address of the backend's code metadata region.
		machine::FnMeta *metadata;

		// Keep track of all references that might need to be updated.
		vector<Reference *> references;

		// Information about a single variable.
		struct Var {
			// Variable id.
			nat id;

			// Variable size.
			nat size;
		};

		// Store information about a single scope.
		struct Block {
			// Parent scope if any, otherwise == this block id.
			nat parent;

			// Number of variables in this scope.
			nat variables;

			// All variables in this scope.
			Var variable[1];

			// Create a block.
			static Block *create(const Frame &frame, const code::Block &b);
		};

		// All blocks in this code. The backend keeps track of which is the currently active block.
		vector<Block *> blocks;

		// Update the contents of this binary.
		void set(const Listing &listing);

		void updateReferences(void *addr, const Output &from);
		void updateBlocks(const Frame &from);

		// Destroy a single variable.
		void destroyVariable(const machine::StackFrame &frame, Var var) const;
	};

	// Reference updater for addresses, used within the Binary class itself.
	class BinaryUpdater : public Reference {
	public:
		BinaryUpdater(const Ref &ref, const String &title, cpuNat *at, bool relative);

		virtual void onAddressChanged(void *newAddress);
	private:
		cpuNat *at;
		bool relative;
	};
}
