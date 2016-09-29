#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "Instr.h"
#include "Label.h"
#include "Variable.h"
#include "Block.h"
#include "ValType.h"

namespace code {
	STORM_PKG(core.asm);

	class Arena;

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
	 *
	 * Note: any labels which have not had an instruction added after them are considered
	 * unused. Ie. it is not possible to reference the end of the listing, as that feature is seldom
	 * used. If you need labels to the end, consider adding a dummy dat(0) at the end.
	 *
	 * TODO: Keep track of our return type? This could make ret() simpler to use.
	 */
	class Listing : public Object {
		STORM_CLASS;
	public:
		// Create an empty listing. Optionally associating it with an arena. Doing that makes the
		// listing show backend-specific things in a proper way.
		STORM_CTOR Listing();
		STORM_CTOR Listing(const Arena *arena);

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
			STORM_CTOR Entry(Instr *instr, MAYBE(Array<Label> *) labels);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);

			// The instruction at this point.
			Instr *instr;

			// Labels before this instruction (if any). Only created if any labels are added.
			MAYBE(Array<Label> *) labels;

			// Add a label here.
			void add(Engine &e, Label label);
		};

		// Add instructions and labels.
		Listing &operator <<(Instr *op);
		Listing &operator <<(Label l);

		// Access instructions.
		inline Nat STORM_FN count() const { return code->count(); }
		inline Bool STORM_FN empty() const { return count() == 0; }
		inline Bool STORM_FN any() const { return count() > 0; }
		inline Instr *STORM_FN operator [](Nat id) const { return at(id); }
		inline Instr *at(Nat id) const { return code->at(id).instr; }

		// Give 'id = count()' to access any labels after the last instruction.
		MAYBE(Array<Label> *) STORM_FN labels(Nat id);

		// Create a shell, ie. a Listing containing only the scope information from this listing,
		// thus making variables, blocks and part from this listing valid in the shell as well.
		Listing *STORM_FN createShell(MAYBE(const Arena *) arena) const;

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
		Block STORM_FN root() const;

		// Create a block inside 'parent'. Note that it does not matter if 'parent' is the block or
		// any part inside that block.
		Block STORM_FN createBlock(Part parent);

		// Create a part after 'after'. If 'after' is not the last part of the block, the last part is
		// added instead.
		Part STORM_FN createPart(Part after);

		// Delay the creation of a variable to a later part.
		void STORM_FN delay(Variable v, Part to);

		// Get the variable stored just before 'v' in this stack frame. Within a single block, it
		// just returns the variable added before 'v'. If 'v' is the first variable in that block,
		// the last variable of the previous block is returned. This will give all variables visible
		// at the same time as the start variable of the iteration. Parameters are returned lastly.
		Variable STORM_FN prev(Variable v) const;

		// Get the previous part. If this is the first part in a block, returns the parent part of the block.
		Part STORM_FN prev(Part p) const;

		// Get the first part in the chain. This is the block.
		Block STORM_FN first(Part p) const;

		// Get the next part in the chain. Returns 'invalid' if none exists.
		Part STORM_FN next(Part p) const;

		// Get the last part in the chain.
		Part STORM_FN last(Part p) const;

		// Get the parent part to a variable or a block.
		Part STORM_FN parent(Part b) const;
		Part STORM_FN parent(Variable b) const;

		// See if the variable 'v' is accessible in the part 'p'. This is almost equivalent to
		// checking if any parent blocks of 'p' contains the variable.
		Bool STORM_FN accessible(Variable v, Part p) const;

		// See if the part 'q' is an indirect parent to 'parent'.
		Bool STORM_FN isParent(Block parent, Part q) const;

		// Is this a parameter?
		Bool STORM_FN isParam(Variable v) const;

		// Get all blocks.
		Array<Block> *STORM_FN allBlocks() const;

		// Get all parts.
		Array<Part> *STORM_FN allParts() const;

		// Get all variables. Always in order, so allVars()[i].key() == i
		Array<Variable> *STORM_FN allVars() const;

		// Get all variables in a block.
		Array<Variable> *STORM_FN allVars(Block b) const;

		// Get all variables in a part. Note that there may be more variables in other parts in the
		// same block, which are also visible.
		Array<Variable> *STORM_FN partVars(Part p) const;

		// Get all parameters.
		Array<Variable> *STORM_FN allParams() const;

		// Get the destructor for a variable or a parameter.
		Operand STORM_FN freeFn(Variable v) const;

		// Get when to free a variable or a parameter.
		FreeOpt STORM_FN freeOpt(Variable v) const;

		// Do this block need an exception handler?
		inline Bool STORM_FN exceptionHandler() const { return needEH; }

		/**
		 * Create variables.
		 */

		inline Variable STORM_FN createVar(Part in, Size size) { return createVar(in, size, Operand(), freeDef); }
		inline Variable STORM_FN createVar(Part in, Size size, Operand free) { return createVar(in, size, free, freeDef); }
		Variable STORM_FN createVar(Part in, Size size, Operand free, FreeOpt when);

		inline Variable STORM_FN createParam(ValType type) { return createParam(type, Operand(), freeDef); }
		inline Variable STORM_FN createParam(ValType type, Operand free) { return createParam(type, free, freeDef); }
		Variable STORM_FN createParam(ValType type, Operand free, FreeOpt when);

		/**
		 * Convenience functions.
		 */

		inline Variable STORM_FN createByteVar(Part in) {
			return createVar(in, Size::sByte);
		}
		inline Variable STORM_FN createIntVar(Part in) {
			return createVar(in, Size::sInt);
		}
		inline Variable STORM_FN createLongVar(Part in) {
			return createVar(in, Size::sLong);
		}
		inline Variable STORM_FN createFloatVar(Part in) {
			return createVar(in, Size::sFloat);
		}

		inline Variable STORM_FN createByteParam() {
			return createParam(ValType(Size::sByte, false));
		}
		inline Variable STORM_FN createIntParam() {
			return createParam(ValType(Size::sInt, false));
		}
		inline Variable STORM_FN createLongParam() {
			return createParam(ValType(Size::sLong, false));
		}
		inline Variable STORM_FN createFloatParam() {
			return createParam(ValType(Size::sFloat, true));
		}


		/**
		 * Misc.
		 */

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Arena.
		const Arena *const arena;

	private:
		// Variable information.
		class IVar {
			STORM_VALUE;
		public:
			// Part containing this variable.
			Nat parent;

			// Size of this variable.
			Size size;

			// Is this a parameter?
			Bool isParam;

			// Is this a float value (only relevant for parameters).
			Bool isFloat;

			// Function to call on free.
			Operand freeFn;

			// When to free?
			FreeOpt freeOpt;

			// Create.
			IVar(Nat parent, Size size, Bool isParam, Bool isFloat, Operand freeFn, FreeOpt opt);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
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

		// Labels added at the end (if any).
		Array<Label> *nextLabels;

		// Next free label id.
		Nat nextLabel;

		// Store variables, parts and blocks. Note that part and block ids refer into the 'parts' array.
		Array<Nat> *params;
		Array<IVar> *vars;
		Array<IBlock> *blocks;
		Array<IPart> *parts;

		// Do we need an exception handler.
		Bool needEH;

		// Find the block id for a part.
		Nat findBlock(Nat partId) const;

		// Find an ID in a Nat array. Returns size if none exists.
		static Nat findId(Array<Nat> *in, Nat val);

		// Create a variable from its index.
		Variable createVar(Nat index) const;

		/**
		 * Output helpers.
		 */

		void putBlock(StrBuf &to, Nat block) const;
		void putPart(StrBuf &to, Nat part, Bool header) const;
		void putVar(StrBuf &to, Nat var) const;
	};

	StrBuf &STORM_FN operator <<(StrBuf &to, Listing::Entry e);

}
