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

	// An implementation of the variable and block management, used by "Listing".
	class Frame : public Printable {
	public:
		Frame();

		// Get the root block.
		static Block root();

		// Create a child block to "root".
		Block createChild(Block parent);

		// Create a variable in a block. Free will be called once with this value as a parameter if
		// the size is <= sizeof(cpuNat), or a pointer to it otherwise.
		inline Variable createByteVar(Block in, Value free = Value(), FreeOpt on = freeDef) {
			return createVariable(in, Size::sByte, free, on);
		}
		inline Variable createIntVar(Block in, Value free = Value(), FreeOpt on = freeDef) {
			return createVariable(in, Size::sInt, free, on);
		}
		inline Variable createLongVar(Block in, Value free = Value(), FreeOpt on = freeDef) {
			return createVariable(in, Size::sLong, free, on);
		}
		inline Variable createPtrVar(Block in, Value free = Value(), FreeOpt on = freeDef) {
			return createVariable(in, Size::sPtr, free, on);
		}

		// Custom variable creation.
		Variable createVariable(Block in, Size size, Value free = Value(), FreeOpt when = freeDef);

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

		// Is 'v' a parameter?
		bool isParam(Variable v) const;

		// What shall be called when the variable goes out of scope?
		Value freeFn(Variable v) const;

		// When should 'freeFn' be used?
		FreeOpt freeOpt(Variable v) const;

		// Get the parent block to "b".
		Block parent(Variable v) const;
		Block parent(Block b) const;

		// Check if 'parent' directly or indirectly contains 'child'.
		bool indirectParent(Block parent, Block child) const;

		// Check if 'v' outlives the block 'b'.
		bool outlives(Variable v, Block b) const;

		// Get all blocks.
		vector<Block> allBlocks() const;

		// Get all children to 'b'.
		vector<Block> children(Block b) const;

		// Get all variables.
		vector<Variable> allVariables() const;

		// Get all variables in "b".
		vector<Variable> variables(Block b) const;

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
			// The containing block.
			nat block;

			// The size of this variable (0 = pointer)
			Size size;

			// Which function to call on free?
			Value freeFn;

			// Free when?
			FreeOpt freeOpt;
		};

		struct InternalBlock {
			nat parent;

			vector<nat> variables;

			InternalBlock() : parent(Block::invalid.id) {}
		};

		// The root block always has id=0.
		nat nextBlockId, nextVariableId;

		// All parameters. (sorted since order is important here!)
		typedef map<nat, Param> ParamMap;
		ParamMap parameters;

		// Stores each block.
		typedef hash_map<nat, InternalBlock> BlockMap;
		BlockMap blocks;

		// Stores all variables.
		typedef hash_map<nat, Var> VarMap;
		VarMap vars;

		// Any custom destruction?
		bool anyDestructors;

		// Get the size of a variable.
		Size size(nat v) const;

		// Helper to the output function.
		void outputBlock(wostream &to, Block b) const;
	};

}
