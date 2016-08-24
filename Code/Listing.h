#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Instr.h"
#include "Label.h"
#include "Variable.h"
#include "Block.h"

namespace code {
	STORM_PKG(core.asm);

	/**
	 * Control when and how variables are freed.
	 */
	enum FreeOpt {
		freeOnNone = 0x0,
		freeOnException = 0x1,
		freeOnBlockExit = 0x2,
		freeOnBoth = 0x3,

		// Pass a pointer to the free-function?
		freePtr = 0x10,

		// Default options.
		freeDef = freeOnBoth,
	};

	BITMASK_OPERATORS(FreeOpt);

	wostream &operator <<(wostream &to, FreeOpt o);
	StrBuf &operator <<(StrBuf &to, FreeOpt o);


	/**
	 * Represents a code listing along with information about blocks and variables. Can be linked
	 * into machine code.
	 *
	 * A listing contains a set of labels. Each instruction may have zero or more labels attached to
	 * them. The labels can then be referenced inside that listing, which creates a reference to the
	 * start of the attached instruction.
	 *
	 * A listing also contains a set of blocks. By default, there is a root block. Each block may
	 * then contain zero or more nested blocks, zero or more parts and zero or more variables. The
	 * root block is active from the prolog() instruction to the epilog() instruction. Whenever a
	 * block is active, all variables inside it and its parent blocks are accessible.
	 *
	 * Each block has a number of parts. These parts are a convenient way of telling which variables
	 * should be automatically destroyed at a given point in the code. When a block is active, all
	 * variables in all parts of that block are accessible to the program. However, we do not
	 * consider destroying the variables until their part has been activated. This is used to
	 * properly handle values, where you generally pass the memory location to a constructor before
	 * you should attempt to execute any destructors there. This scenario is not possible to do with
	 * blocks, as the memory is not ready before the block has been activated and the exception is
	 * activated before the memory has been initialized.
	 */
	class Listing : public Object {
		STORM_CLASS;
	public:
		// Create an empty listing.
		STORM_CTOR Listing();

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

		/**
		 * Labels.
		 */

		// Label reserved for metadata.
		Label STORM_FN meta();

		// Create a new label.
		Label STORM_FN label();

		/**
		 * Scope management.
		 */

		// Get the root block.
		Block STORM_FN root();

		// Create a block inside 'parent'. Note that it does not matter if 'parent' is the block or
		// any part inside that block.
		Block STORM_FN createBlock(Part parent);

		// Create a part after 'after'. If 'after' is not the last part of the block, the last part is
		// added instead.
		Part STORM_FN createPart(Part after);

		// Delay the creation of a variable to a later part.
		void STORM_FN delay(Variable v, Part to);

		// Get the previous variable. Either in the current part, or in the parent block.
		Variable STORM_FN prev(Variable v) const;

		// Get the previous part. If this is the first part in a block, returns the parent part of the block.
		Part STORM_FN prev(Part p) const;

		// Get the first part in the chain. This is the block.
		Block STORM_FN first(Part p) const;

		// Get the next part in the chain. Returns 'invalid' if none exists.
		Part STORM_FN next(Part p) const;

		// Get the last part in the chain.
		Part STORM_FN last(Part p) const;

		/**
		 * Create variables.
		 */

		inline Variable STORM_FN createVar(Part in, Size size) { return createVar(in, size, Operand(), freeDef); }
		inline Variable STORM_FN createVar(Part in, Size size, Operand free) { return createVar(in, size, free, freeDef); }
		Variable STORM_FN createVar(Part in, Size size, Operand free, FreeOpt when);

		/**
		 * Misc.
		 */

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

	private:
		// Variable information.
		class IVar {
			STORM_VALUE;
		public:
			// Part containing this variable.
			Nat parent;

			// Size of this variable.
			Size size;

			// Is this a float value (only relevant for parameters).
			Bool isFloat;

			// Function to call on free.
			Operand freeFn;

			// When to free?
			FreeOpt freeOpt;

			// Create.
			IVar(Nat parent, Size size, Bool isFloat, Operand freeFn, FreeOpt opt);
		};

		// Block information.
		class IBlock {
			STORM_VALUE;
		public:
			// Parent block.
			Nat parent;

			// All parts in this block.
			Array<Nat> *parts;

			// Create.
			IBlock(Engine &e);
			IBlock(Engine &e, Nat parent);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
		};

		// Part information.
		class IPart {
			STORM_VALUE;
		public:
			// Inside this block (= first part in the chain).
			Nat block;

			// Our index in the chain.
			Nat index;

			// Variables declared here.
			Array<Nat> *vars;

			// Create.
			IPart(Engine &e, Nat block, Nat index);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
		};

		// Instructions and labels in here.
		Array<Entry> *code;

		// Next free label id.
		Nat nextLabel;

		// Store variables, parts and blocks. Note that part and block ids refer into the 'parts' array.
		Array<IVar> *vars;
		Array<IBlock> *blocks;
		Array<IPart> *parts;

		// Find the block id for a part.
		Nat findBlock(Nat partId) const;

		/**
		 * Output helpers.
		 */

		void putBlock(StrBuf &to, Nat block) const;
		void putPart(StrBuf &to, Nat part, Bool header) const;
		void putVar(StrBuf &to, Nat var) const;
	};

	StrBuf &STORM_FN operator <<(StrBuf &to, Listing::Entry e);

}
