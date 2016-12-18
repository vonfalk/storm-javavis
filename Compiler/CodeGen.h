#pragma once
#include "Code/Listing.h"
#include "RunOn.h"

namespace storm {
	STORM_PKG(core.lang);

	class CodeGen;
	class Function;

	/**
	 * Information about a created variable. It contains:
	 * - the variable itself
	 * - if the variable needs a specific part for itself
	 *
	 * In the case of values, they need to be allocated in a separate part so that their destructor
	 * is not executed too early in case of an exception.
	 */
	class VarInfo {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR VarInfo();

		// Create, assume we do not need anything special.
		STORM_CTOR VarInfo(code::Var v);

		// Create, set flags.
		STORM_CTOR VarInfo(code::Var v, Bool needsPart);

		// Variable.
		code::Var v;

		// Needs a specific part after constructed?
		Bool needsPart;

		// Do anything needed when the variable has been created.
		void STORM_FN created(CodeGen *gen);
	};


	/**
	 * Class that contains the state of the code generation in many languages. Has basic support for
	 * return values as well.
	 *
	 * TODO: Make a toS!
	 */
	class CodeGen : public Object {
		STORM_CLASS;
	public:
		// Create. Supply which thread we're supposed to run on.
		STORM_CTOR CodeGen(RunOn thread);

		// Create a new CodeGen which attaches to an already existing listing.
		STORM_CTOR CodeGen(RunOn thread, code::Listing *l);
		STORM_CTOR CodeGen(RunOn thread, code::Listing *l, code::Block block);

		// Create a child.
		STORM_CTOR CodeGen(CodeGen *me, code::Block b);

		// Create a child code gen where another block is the topmost one.
		CodeGen *STORM_FN child(code::Block block);

		// Create a child code gen with a new child block.
		CodeGen *STORM_FN child();

		// Deep copy.
		virtual void STORM_FN deepCopy(CloneEnv *env);

		/**
		 * Create parameters (aware of Storm semantics).
		 */

		// Add a parameter of 'type'.
		code::Var STORM_FN createParam(Value type);

		/**
		 * Create variables (aware of Storm semantics).
		 */

		// Add a variable of 'type' in the current block.
		VarInfo STORM_FN createVar(Value type);

		// Add a variable of 'type' in 'block'.
		VarInfo STORM_FN createVar(Value type, code::Block in);


		/**
		 * Return values.
		 */

		// Set the return type. This needs to be done after all parameters have been added.
		void STORM_FN result(Value type, Bool isMember);

		// Get the return type.
		Value STORM_FN result() const;

		// Return the value stored in 'value'. Generates an epilog and a ret instruction.
		void STORM_FN returnValue(code::Var value);

		/**
		 * Data.
		 */

		// Output to here.
		code::Listing *to;

		// Which thread will this code be running on?
		RunOn runOn;

		// Current block.
		code::Block block;

	private:
		// Return type.
		Value res;

		// Return parameter.
		code::Var resParam;
	};


	/**
	 * Resulting value from generated code. It encapsulates a variable (created on demand) that will
	 * store the result from another part of code generation (usually a sub-block).
	 *
	 * This class allows the caller to explicitly ask for the result to be stored in a specific
	 * location, or it can leave that decision up to the calee or to this class.
	 *
	 * TODO: Make toS and deepCopy (no-op).
	 */
	class CodeResult : public Object {
		STORM_CLASS;
	public:
		// No result is needed.
		STORM_CTOR CodeResult();

		// A result of type 't' is needed in 'block'. A variable is created as needed.
		STORM_CTOR CodeResult(Value type, code::Block block);

		// A result of type 't' should be stored in 'var'.
		STORM_CTOR CodeResult(Value type, code::Var var);
		STORM_CTOR CodeResult(Value type, VarInfo var);

		// Get a location to store the variable in. Asserts if the result is not needed.
		VarInfo STORM_FN location(CodeGen *s);

		// Get the location of the result even if the result is not needed.
		VarInfo STORM_FN safeLocation(CodeGen *s, Value type);

		// Suggest a location for the result. Returns true if the suggestion is acceptable,
		// otherwise use whatever 'location' returns.
		Bool STORM_FN suggest(CodeGen *s, code::Var v);
		Bool STORM_FN suggest(CodeGen *s, code::Operand v);

		// What type is needed? (void = not needed).
		Value STORM_FN type() const;

		// Result needed?
		Bool STORM_FN needed() const;

	private:
		// Variable (invalid if not created yet).
		VarInfo variable;

		// Block 'variable' is needed inside.
		code::Block block;

		// Type.
		Value t;
	};

	// Generate code to fill in a BasicTypeInfo struct. Only touches eax register.
	code::Var createBasicTypeInfo(CodeGen *to, Value v);

	// Generate code that creates a FnParams object on the stack. Issueas a function call, and does not
	// preserve any register values. 'memory' will be passed to the FnParams ctor as the memory to be used.
	// Uses 'ptrC' before 'memory' is used.
	code::Var STORM_FN createFnParams(CodeGen *s, code::Operand memory);

	// Same as above, but takes the number of parameters needed to be passed.
	code::Var STORM_FN createFnParams(CodeGen *s, Nat numParams);

	// Add a parameter to a FnParams object.
	void STORM_FN addFnParam(CodeGen *s, code::Var fnParams, Value type, code::Operand v);

	// Add a copied parameter to a FnParams object. May add variables to the current scope.
	void STORM_FN addFnParamCopy(CodeGen *s, code::Var fnParams, Value type, code::Operand v);

	// Allocate an object on the heap. Store it in variable 'to'.
	void STORM_FN allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to);
	code::Var STORM_FN allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params);

}
