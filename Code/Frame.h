#pragma once

#include "Block.h"
#include "Value.h"

#include <vector>
#include <map>
#include "Utils/HashMap.h"
#include "Utils/Bitmask.h"

namespace code {

	/**
	 * Control when and how variables are freed.
	 */
	enum FreeOpt {
		freeOnNone = 0x0,
		freeOnException = 0x1,
		freeOnBlockExit = 0x2,
		freeOnBoth = 0x3,

		// Pass a pointer to the free function? (by-value is default).
		freePtr = 0x10,

		// Default options.
		freeDef = freeOnBoth,
	};

	// And+or.
	BITMASK_OPERATORS(FreeOpt);

	// ToString for 'FreeOpt'
	String name(FreeOpt f);

	/**
	 * Variable and block management, used by 'Listing'.
	 *
	 * The block management is heavily inspired from how C and C++ handles blocks.
	 * A block is a set of code that uses a number of variables. These blocks are in turn
	 * linked into a tree. Variables declared in one block are visible from all its children,
	 * but not the other way around. Consider the following tree:
	 * root
	 *  |- var0
	 *  |- A
	 *  |\- var1
	 *  \- B
	 *   \- var2
	 * Here, var0 is accessible from root, A and B while var1 is only accessible from A. When
	 * running, the generated machine code will reuse the space used for 'var1' for 'var2' since
	 * we know that they can never exist at the same time.
	 *
	 * Aside from creating blocks as a tree, it is also possible to create them in a linear
	 * fashion. To explain the need, consider the following code snippnet:
	 * {
	 *     Foo a;
	 *     // ...
	 *     Foo b;
	 *     // ...
	 * }
	 * Here, 'a' and 'b' will be in the same block, which is fine except for exception handling.
	 * If there would be an exception fired between the declaration of 'a' and 'b', 'b's exception
	 * handler will be executed even though it has not yet been initialized. One solution for this
	 * would be to re-structure the code like this:
	 * {
	 *     Foo a;
	 *     // ...
	 *     {
	 *         Foo b;
	 *         // ...
	 *     }
	 * }
	 * This has several disadvantages. Firstly, the generated tree will be unneccessarily deep, which
	 * makes debugging more difficult. Secondly, implementing the generations of scopes like this correctly
	 * is difficult at compiler level, and thirdly, it is commonly neccessary to allocate storage for a
	 * variable before the exception handler is activated (for example, when returning non-primitives).
	 * Because of this, a second type of block is supported, called parts. This type of block simply
	 * divides a block into different parts, where each part may add one or more exception handlers to
	 * the variables of the block. In all other aspects they act like a regular block. However, this kind
	 * of block can not be end()ed. Note that parts are sequential by nature. The first part in a Block
	 * can simply be casted from a Block id.
	 *
	 * Note that it is generally assumed that jumps do not alter which the current block is!
	 */
	class Frame : public Printable {
	public:
		Frame();

		// Get the root block.
		static Block root();

		// Create a child block to 'parent'.
		Block createChild(Part parent);

		// Extend the block to contain another sub-section.
		Part createPart(Part after);

		// Create a variable in a block. Free will be called once with this value as a parameter if
		// the size is <= sizeof(cpuNat), or a pointer to it otherwise.
		inline Variable createByteVar(Part in, Value free = Value(), FreeOpt on = freeDef) {
			return createVariable(in, Size::sByte, free, on);
		}
		inline Variable createIntVar(Part in, Value free = Value(), FreeOpt on = freeDef) {
			return createVariable(in, Size::sInt, free, on);
		}
		inline Variable createLongVar(Part in, Value free = Value(), FreeOpt on = freeDef) {
			return createVariable(in, Size::sLong, free, on);
		}
		inline Variable createPtrVar(Part in, Value free = Value(), FreeOpt on = freeDef) {
			return createVariable(in, Size::sPtr, free, on);
		}

		// Custom variable creation.
		Variable createVariable(Part in, Size size, Value free = Value(), FreeOpt when = freeDef);

		// Create a function parameter. These are assumed to be
		// created in the same order as the parameters appear in the function
		// declaration in C/C++. IsFloat shall be true if the parameter is
		// a floating-point parameter. In some calling conventions, floating
		// point parameters are treated separately.
		inline Variable createByteParam(Value free = Value(), FreeOpt on = freeDef) {
			return createParameter(Size::sByte, false, free, on);
		}
		inline Variable createIntParam(Value free = Value(), FreeOpt on = freeDef) {
			return createParameter(Size::sInt, false, free, on);
		}
		inline Variable createLongParam(Value free = Value(), FreeOpt on = freeDef) {
			return createParameter(Size::sLong, false, free, on);
		}
		inline Variable createPtrParam(Value free = Value(), FreeOpt on = freeDef) {
			return createParameter(Size::sPtr, false, free, on);
		}

		// Custom parameter creation.
		Variable createParameter(Size size, bool isFloat, Value free = Value(), FreeOpt when = freeDef);

		// Get the variable located before the current variable (either in the same, or in another block).
		// Returns an empty variable if none exists.
		Variable prev(Variable v) const;

		// Get the previous Part stored. If 'p' is the first one of the block, it returns the last
		// part of the parent block. So by calling this repeatedly, you will iterate through
		// all parts that is _visible at the same time_ as p.
		Part prevStored(Part p) const;

		// Get the previous Part. If 'p' is the first part of the block, the block's parent is returned.
		Part prev(Part p) const;

		// Is 'v' a parameter?
		bool isParam(Variable v) const;

		// What shall be called when the variable goes out of scope?
		Value freeFn(Variable v) const;

		// When should 'freeFn' be used?
		FreeOpt freeOpt(Variable v) const;

		// Get the last part in the chain.
		Part last(Part p) const;

		// Get the next part in the chain.
		Part next(Part p) const;

		// Get the first part in the chain.
		Block first(Part p) const;

		// Get the block 'p' represents, if any.
		Block asBlock(Part p) const;

		// Get the parent part to "b".
		Part parent(Block b) const;
		Part parent(Variable v) const;

		// Check if 'parent' directly or indirectly contains 'child'.
		bool indirectParent(Block parent, Block child) const;

		// Check if block 'p' can access variable 'v'. This is equivalent of
		// checking wether any of the parents to 'p' contains the variable 'v'.
		bool accessible(Part p, Variable v) const;

		// Get all blocks.
		vector<Block> allBlocks() const;

		// Get all parts.
		vector<Part> allParts() const;

		// Get all children to 'b'. Note that the next block is not considered a child.
		vector<Block> children(Part b) const;

		// Get all variables.
		vector<Variable> allVariables() const;

		// Get all variables in a block.
		vector<Variable> allVariables(Block b) const;

		// Get all variables in "b". Note that there may be more variables in other Parts!
		vector<Variable> variables(Part p) const;

		// Get the total number of parameters in this block manager.
		nat parameterCount() const;

		// Is an exception handler needed for this frame?
		bool exceptionHandlerNeeded() const;

	protected:
		virtual void output(wostream &to) const;

	private:
		struct Param {
			// The size of this parameter.
			Size size;

			// Is it a floating-point parameter?
			bool isFloat;

			// Which function to call on free?
			Value freeFn;

			// Free when?
			FreeOpt freeOpt;
		};

		struct Var {
			// The containing part.
			nat part;

			// The size of this variable (0 = pointer)
			Size size;

			// Which function to call on free?
			Value freeFn;

			// Free when?
			FreeOpt freeOpt;
		};

		struct InternalBlock {
			// Parent part.
			nat parent;

			InternalBlock() : parent(Block::invalid.id) {}
			InternalBlock(nat parent) : parent(parent) {}
		};

		struct InternalPart {
			// Inside this block. (the first part in the chain).
			nat block;

			// Previous part in the chain.
			nat prev;

			// The next part in the chain.
			nat next;

			// Variables declared here.
			vector<nat> variables;

			InternalPart() : block(Part::invalid.id), prev(Part::invalid.id), next(Part::invalid.id) {}
			InternalPart(nat block) : block(block), prev(Part::invalid.id), next(Part::invalid.id) {}
			InternalPart(nat block, nat prev) : block(block), prev(prev), next(Part::invalid.id) {}
		};

		// The root block always has id=0.
		nat nextPartId, nextVariableId;

		// All parameters. (sorted since order is important here!)
		typedef map<nat, Param> ParamMap;
		ParamMap parameters;

		// Stores each block.
		typedef hash_map<nat, InternalBlock> BlockMap;
		BlockMap blocks;

		// Stores each part.
		typedef hash_map<nat, InternalPart> PartMap;
		PartMap parts;

		// Stores all variables.
		typedef hash_map<nat, Var> VarMap;
		VarMap vars;

		// Any custom destruction?
		bool anyDestructors;

		// Get the size of a variable.
		Size size(nat v) const;

		// Helper to the output function.
		void outputBlock(wostream &to, Block b) const;

		// Get all variables in a part. Append to an array.
		void variables(Part p, vector<Variable> &to) const;
	};

}
