#pragma once

#include "Block.h"
#include "Value.h"

#include <vector>
#include <map>
#include "Utils/HashMap.h"

namespace code {

	// An implementation of the variable and block management, used by "Listing".
	class Frame {
	public:
		Frame();

		// Get the root block.
		static Block root();

		// Create a child block to "root".
		Block createChild(Block parent);

		// Create a variable in a block. Free will be called once with this value as a parameter if
		// the size is <= sizeof(cpuNat), or a pointer to it otherwise.
		inline Variable createByteVar(Block in, Value free = Value()) { return createVariable(in, 1, free); }
		inline Variable createIntVar(Block in, Value free = Value()) { return createVariable(in, 4, free); }
		inline Variable createLongVar(Block in, Value free = Value()) { return createVariable(in, 8, free); }
		inline Variable createPtrVar(Block in, Value free = Value()) { return createVariable(in, 0, free); }

		// Custom variable creation.
		Variable createVariable(Block in, nat size, Value free = Value());

		// Create a function parameter. These are assumed to be
		// created in the same order as the parameters appear in the function
		// declaration in C/C++. IsFloat shall be true if the parameter is
		// a floating-point parameter. In some calling conventions, floating
		// point parameters are treated separately.
		inline Variable createByteParam(Value free = Value()) { return createParameter(1, false, free); }
		inline Variable createIntParam(Value free = Value()) { return createParameter(4, false, free); }
		inline Variable createLongParam(Value free = Value()) { return createParameter(8, false, free); }
		inline Variable createPtrParam(Value free = Value()) { return createParameter(0, false, free); }

		// Custom parameter creation.
		Variable createParameter(nat size, bool isFloat, Value free = Value());

		// Get the variable located before the current variable (either in the same, or in another block).
		// Returns an empty variable if none exists.
		Variable prev(Variable v) const;

		// Is 'v' a parameter?
		bool isParam(Variable v) const;

		// What shall be called when the variable goes out of scope?
		Value freeFn(Variable v) const;

		// Get the parent block to "b".
		Block parent(Variable v) const;
		Block parent(Block b) const;

		// Get all blocks.
		vector<Block> allBlocks() const;

		// Get all variables.
		vector<Variable> allVariables() const;

		// Get all variables in "b".
		vector<Variable> variables(Block b) const;

		// Get the total number of parameters in this block manager.
		nat parameterCount() const;

		// Is an exception handler needed for this frame?
		bool exceptionHandlerNeeded() const;
	private:
		struct Param {
			// The size of this parameter.
			nat size;

			// Is it a floating-point parameter?
			bool isFloat;

			// Which function to call on free?
			Value freeFn;
		};

		struct Var {
			// The containing block.
			nat block;

			// The size of this variable (0 = pointer)
			nat size;

			// Which function to call on free?
			Value freeFn;
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
		nat size(nat v) const;
	};

}
