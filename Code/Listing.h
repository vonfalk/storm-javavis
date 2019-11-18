#pragma once
#include "Core/Object.h"
#include "Core/Array.h"
#include "TypeDesc.h"
#include "Instr.h"
#include "Label.h"
#include "Var.h"
#include "Block.h"

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

		// Used by the backends: What is stored in the variable is actually a pointer to the value.
		freeIndirection = 0x20,

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
		STORM_CTOR Listing(Bool member, TypeDesc *result);
		STORM_CTOR Listing(const Arena *arena);
		STORM_CTOR Listing(const Arena *arena, Bool member, TypeDesc *result);

		Listing(const Listing &o);

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		/**
		 * One entry in the listing:
		 */
		class Entry {
			STORM_VALUE;
		public:
			STORM_CTOR Entry();
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
		Listing &STORM_FN operator <<(Instr *op);
		Listing &STORM_FN operator <<(Label l);
		Listing &STORM_FN operator <<(MAYBE(Array<Label> *) l);

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
		Listing *STORM_FN createShell() const;
		Listing *STORM_FN createShell(MAYBE(const Arena *) arena) const;

		/**
		 * Labels.
		 */

		// Label reserved for metadata.
		Label STORM_FN meta();

		// Create a new label.
		Label STORM_FN label();

		/**
		 * Additional source-level information about variables. Front-ends may tag variables in this
		 * way to provide a better debugging experience, but doing so is optional.
		 */
		class VarInfo : public Object {
			STORM_CLASS;
		public:
			// Name of the variable.
			Str *name;

			// Type of the variable.
			Type *type;

			// Source location.
			SrcPos pos;

			// Create.
			STORM_CTOR VarInfo(Str *name, Type *type, SrcPos pos);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
		};

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
		void STORM_FN delay(Var v, Part to);

		// Move a parameter to a specific location.
		void STORM_FN moveParam(Var param, Nat to);

		// Get the variable stored just before 'v' in this stack frame. Within a single block, it
		// just returns the variable added before 'v'. If 'v' is the first variable in that block,
		// the last variable of the previous block is returned. This will give all variables visible
		// at the same time as the start variable of the iteration. Parameters are returned lastly.
		Var STORM_FN prev(Var v) const;

		// Get the previous part. If this is the first part in a block, returns the parent part of the block.
		Part STORM_FN prev(Part p) const;

		// Get the previous stored part. Same as 'prev' except when 'p' is the first part in a
		// block, it returns the *last* part in the parent block.
		Part STORM_FN prevStored(Part p) const;

		// Get the first part in the chain. This is the block.
		Block STORM_FN first(Part p) const;

		// Get the next part in the chain. Returns 'invalid' if none exists.
		Part STORM_FN next(Part p) const;

		// Get the last part in the chain.
		Part STORM_FN last(Part p) const;

		// Get the parent part to a variable or a block.
		Part STORM_FN parent(Part b) const;
		Part STORM_FN parent(Var b) const;

		// See if the variable 'v' is accessible in the part 'p'. This is almost equivalent to
		// checking if any parent blocks of 'p' contains the variable.
		Bool STORM_FN accessible(Var v, Part p) const;

		// See if the part 'q' is an indirect parent to 'parent'.
		Bool STORM_FN isParent(Block parent, Part q) const;

		// Is this a parameter?
		Bool STORM_FN isParam(Var v) const;

		// Get parameter info about a variable.
		MAYBE(TypeDesc *) STORM_FN paramDesc(Var v) const;

		// Get debug info about a variable.
		MAYBE(VarInfo *) STORM_FN varInfo(Var v) const;

		// Set debug info.
		void STORM_FN varInfo(Var v, MAYBE(VarInfo *) info);

		// Get all blocks.
		Array<Block> *STORM_FN allBlocks() const;

		// Get all parts.
		Array<Part> *STORM_FN allParts() const;

		// Get all variables. Always in order, so allVars()[i].key() == i
		Array<Var> *STORM_FN allVars() const;

		// Get all variables in a block.
		Array<Var> *STORM_FN allVars(Block b) const;

		// Get all variables in a part. Note that there may be more variables in other parts in the
		// same block, which are also visible.
		Array<Var> *STORM_FN partVars(Part p) const;

		// Get all parameters.
		Array<Var> *STORM_FN allParams() const;

		// Get the destructor for a variable or a parameter.
		Operand STORM_FN freeFn(Var v) const;

		// Get when to free a variable or a parameter.
		FreeOpt STORM_FN freeOpt(Var v) const;
		void STORM_FN freeOpt(Var v, FreeOpt opt);

		// Do this block need an exception handler?
		inline Bool STORM_FN exceptionHandler() const { return needEH; }

		/**
		 * Create variables.
		 */

		inline Var STORM_FN createVar(Part in, Size size) { return createVar(in, size, Operand(), freeDef); }
		inline Var STORM_FN createVar(Part in, Size size, Operand free) { return createVar(in, size, free, freeDef); }
		Var STORM_FN createVar(Part in, Size size, Operand free, FreeOpt when);
		Var STORM_FN createVar(Part in, TypeDesc *type);
		Var STORM_FN createVar(Part in, TypeDesc *type, FreeOpt when);

		Var STORM_FN createParam(TypeDesc *type);
		Var STORM_FN createParam(TypeDesc *type, FreeOpt when);
		Var STORM_FN createParam(TypeDesc *type, Operand free);
		Var STORM_FN createParam(TypeDesc *type, Operand free, FreeOpt when);

		/**
		 * Convenience functions.
		 */

		inline Var STORM_FN createByteVar(Part in) {
			return createVar(in, Size::sByte);
		}
		inline Var STORM_FN createIntVar(Part in) {
			return createVar(in, Size::sInt);
		}
		inline Var STORM_FN createLongVar(Part in) {
			return createVar(in, Size::sLong);
		}
		inline Var STORM_FN createFloatVar(Part in) {
			return createVar(in, Size::sFloat);
		}

		inline Var STORM_FN createByteParam() {
			return createParam(new (this) PrimitiveDesc(bytePrimitive()));
		}
		inline Var STORM_FN createIntParam() {
			return createParam(new (this) PrimitiveDesc(intPrimitive()));
		}
		inline Var STORM_FN createPtrParam() {
			return createParam(new (this) PrimitiveDesc(ptrPrimitive()));
		}
		inline Var STORM_FN createLongParam() {
			return createParam(new (this) PrimitiveDesc(longPrimitive()));
		}
		inline Var STORM_FN createFloatParam() {
			return createParam(new (this) PrimitiveDesc(floatPrimitive()));
		}

		/**
		 * Result from this listing.
		 */

		// Resulting type from this listing.
		TypeDesc *result;

		// Is this function a member of a class?
		Bool member;


		/**
		 * Misc.
		 */

		// Output.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Arena.
		MAYBE(const Arena *const) arena;

	private:
		// Variable information.
		class IVar {
			STORM_VALUE;
		public:
			// Part containing this variable.
			Nat parent;

			// Size of this variable.
			Size size;

			// Parameter? If so: description of the parameter.
			MAYBE(TypeDesc *) param;

			// Optional variable information.
			MAYBE(VarInfo *) info;

			// Function to call on free.
			Operand freeFn;

			// When to free?
			FreeOpt freeOpt;

			// Create.
			IVar(Nat parent, Size size, TypeDesc *param, Operand freeFn, FreeOpt opt);

			// Deep copy.
			void STORM_FN deepCopy(CloneEnv *env);
		};

		// Block information.
		class IBlock {
			STORM_VALUE;
		public:
			// Parent part.
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

		// Initialize everything.
		void init(Bool member, TypeDesc *result);

		// Find the block id for a part.
		Nat findBlock(Nat partId) const;

		// Find an ID in a Nat array. Returns size if none exists.
		static Nat findId(Array<Nat> *in, Nat val);

		// Create a variable from its index.
		Var createVar(Nat index) const;

		/**
		 * Output helpers.
		 */

		void putBlock(StrBuf &to, Nat block) const;
		void putPart(StrBuf &to, Nat part, Bool header) const;
		void putVar(StrBuf &to, Nat var) const;
	};

	StrBuf &STORM_FN operator <<(StrBuf &to, Listing::Entry e);

}

#include "InstrFwd.h"
