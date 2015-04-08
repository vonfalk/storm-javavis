#pragma once
#include "Code/Listing.h"
#include "Value.h"
#include "CodeData.h"
#include "NamedThread.h"
#include "AsmWrap.h"

namespace storm {
	/**
	 * ---------------- OLD CODE -------------------------
	 */


	/**
	 * Describes the state of code generation.
	 */
	struct GenState {
		// Generate code here.
		code::Listing &to;

		// Generate any data here.
		CodeData &data;

		// Which thread are we running on?
		RunOn runOn;

		// Current frame (generally points to to.frame).
		code::Frame &frame;

		// Current part.
		code::Block block;

		// Create a child GenState.
		inline GenState child(code::Block block) const {
			GenState s = {
				to,
				data,
				runOn,
				frame,
				block,
			};
			return s;
		}
	};


	/**
	 * --------------- END OLD CODE -------------------
	 */


	/**
	 * Class that contains the state for code generation in many languages.
	 */
	class CodeGen : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// Create a new code generation object. Creates all needed contents.
		// 'thread' indicates the thread we expect the generated machine code to run on, and
		// may be used to decide on what kind of code to generate.
		STORM_CTOR CodeGen(RunOn thread);

		// Copy ctor (shallow).
		STORM_CTOR CodeGen(Par<CodeGen> o);

		// Create a child CodeGen object. It is the same as this one except that
		// the Block member points to a different block.
		CodeGen *STORM_FN child(wrap::Block block);

		/**
		 * Members (these are the wrapper versions, so that Storm may access them).
		 */

		// Output listing.
		Auto<wrap::Listing> l;

		// Output data.
		Auto<CodeData> data;

		// Which thread will this code be running on?
		RunOn runOn;

		// Current block.
		wrap::Block block;

		// Convenient access from C++.
		code::Listing &to; // points to 'l->v'
		code::Frame &frame; // points to 'to.frame'
	};


	/**
	 * Information about a created variable. This contains: 1: the variable itself,
	 * and 2: if the variable needs a specific part for itself. In the case of values, they need
	 * to be allocated in a separate part so that their destructor is not executed too early in
	 * case of an exception.
	 */
	class VarInfo {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR VarInfo();

		// Create. Assume we do not need anything special.
		explicit STORM_CTOR VarInfo(wrap::Variable v);

		// Create. Set flags.
		explicit STORM_CTOR VarInfo(wrap::Variable v, Bool needsPart);

		// Variable.
		STORM_VAR wrap::Variable v;

		// Get unwrapped variable.
		inline code::Variable var() const { return v.v; }

		// Needs a specific part after constructed?
		STORM_VAR Bool needsPart;

		// Do anything needed when the variable has been created.
		void STORM_FN created(Par<CodeGen> gen);

		// Mute some errors while we're working... Remove this one later!
		void created(GenState s);
	};


	/**
	 * Result from generating code. It encapsulates a variable (created on demand) that will store
	 * the result from another part of code generation. Usually from a sub-block.
	 */
	class CodeResult : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		// No result is needed.
		STORM_CTOR CodeResult();

		// A result of type 't' is needed in 'block'. A variable is created as needed.
		STORM_CTOR CodeResult(const Value &t, wrap::Block block);

		// A result of type 't' should be stored in 't'.
		STORM_CTOR CodeResult(const Value &t, VarInfo var);

		// Get a location to store the variable in, assuming the result is needed.
		VarInfo location(Par<CodeGen> s);

		// Get the location of the result, even if it is not needed by the creator.
		VarInfo safeLocation(Par<CodeGen> s);

		// Suggest a location. Returns true if it should be used. Otherwise, store the value in
		// whatever location is returned by 'location' instead.
		Bool STORM_FN suggest(Par<CodeGen> s, wrap::Variable v);
		Bool STORM_FN suggest(Par<CodeGen> s, wrap::Operand v);

		// What type is needed? (void = not needed).
		inline Value STORM_FN type() const { return t; }

		// Result needed?
		inline Bool STORM_FN needed() const { return t != Value(); }

	private:
		// Variable (invalid if not created yet).
		VarInfo variable;

		// Block.
		code::Block block;

		// Type.
		Value t;
	};

	// Helper to create a variable from a Value (this is common).
	VarInfo variable(code::Frame &frame, code::Block block, const Value &v);

	// Variations.
	VarInfo STORM_FN variable(wrap::Listing l, wrap::Block block, Value v) {
		return variable(l.v.frame, block.v, value);
	}

	VarInfo STORM_FN variable(Par<CodeGen> g, Value v) {
		return variable(g->frame, g->block.v, v);
	}


	/**
	 * ---------------- BEGIN OLD CODE ----------------------
	 */


	/**
	 * Used to tell code generating functions where to place
	 * the generated result (if any).
	 * The result is either not needed (GenResult()), needed
	 * but unspecified, or needed and specified.
	 * Todo: Allow these to reside in registers as well?
	 */
	class GenResult {
	public:
		// No result is needed.
		GenResult();

		// A result is of type 't' needed in 'block'.
		GenResult(const Value &t, code::Block block);

		// Indicate that the result should be stored in 'var'.
		GenResult(const Value &t, VarInfo var);

		// Get the location of the result, assuming the result is needed.
		VarInfo location(const GenState &s);

		// Get the location of the result, even if it was not required by the creator.
		VarInfo safeLocation(const GenState &s, const Value &t);

		// Suggest a location. Returns true if it is used. Otherwise, store the value in whatever
		// location is returned by 'location'.
		bool suggest(const GenState &s, code::Variable v);
		bool suggest(const GenState &s, code::Value v);

		// What type of result is needed? (void = no result needed at all).
		const Value type;

		// Any result needed?
		inline bool needed() const { return type != Value(); }

	private:
		// Stored variable.
		VarInfo variable;

		// In which block?
		code::Block block;
	};

	inline VarInfo variable(const GenState &s, const Value &v) {
		return variable(s.frame, s.block, v);
	}

	// Generate code to fill in a BasicTypeInfo struct. Only touches eax register.
	code::Variable createBasicTypeInfo(const GenState &to, const Value &v);

	// Create an object on the heap. Store it in variable 'to'. Only work for heap-allocated objects.
	void allocObject(const GenState &s, Function *ctor, vector<code::Value> params, code::Variable to);
	void allocObject(code::Listing &l, code::Block b, Function *ctor, vector<code::Value> params, code::Variable to);

}
