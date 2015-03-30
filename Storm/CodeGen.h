#pragma once
#include "Code/Listing.h"
#include "Value.h"
#include "CodeData.h"
#include "NamedThread.h"

namespace storm {

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
	 * Information about a created variable. This contains: 1: the variable itself,
	 * and 2: the part it was allocated in. In the case of value variables, they have
	 * to be allocated in a separate part so that their destructor is not executed too early.
	 */
	class VarInfo {
	public:
		// Create.
		VarInfo();
		explicit VarInfo(const code::Variable &v);
		VarInfo(code::Variable v, bool needsPart);

		// The variable created.
		code::Variable var;

		// Does this variable need a separate part for itself to correctly handle destructors?
		bool needsPart;

		// Begin 'part' if needed. Call when you have initialized the value.
		void created(const GenState &to);

	};

	// Helper to create a variable from a Value (this is common).
	VarInfo variable(code::Frame &frame, code::Block block, const Value &v);

	// Variations.
	inline VarInfo variable(code::Listing &l, code::Block block, const Value &v) {
		return variable(l.frame, block, v);
	}

	inline VarInfo variable(const GenState &s, const Value &v) {
		return variable(s.frame, s.block, v);
	}

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

	// Generate code to fill in a BasicTypeInfo struct. Only touches eax register.
	code::Variable createBasicTypeInfo(const GenState &to, const Value &v);

	// Create an object on the heap. Store it in variable 'to'. Only work for heap-allocated objects.
	void allocObject(const GenState &s, Function *ctor, vector<code::Value> params, code::Variable to);
}
