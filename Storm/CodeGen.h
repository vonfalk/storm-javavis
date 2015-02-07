#pragma once
#include "Code/Listing.h"
#include "Value.h"
#include "CodeData.h"

namespace storm {

	/**
	 * Describes the state of code generation.
	 */
	struct GenState {
		// Generate code here.
		code::Listing &to;

		// Generate any data here.
		CodeData &data;

		// Current frame (generally points to to.frame).
		code::Frame &frame;

		// Current block.
		code::Block block;
	};

	// Helper to create a variable from a Value (this is common).
	code::Variable variable(code::Frame &frame, code::Block block, const Value &v);

	// Variations.
	inline code::Variable variable(code::Listing &l, code::Block block, const Value &v) {
		return variable(l.frame, block, v);
	}

	inline code::Variable variable(const GenState &s, const Value &v) {
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
		GenResult(const Value &t, code::Variable var);

		// Get the location of the result, assuming the result is needed.
		code::Variable location(const GenState &s);

		// Get the location of the result, even if it was not required by the creator.
		code::Variable safeLocation(const GenState &s, const Value &t);

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
		code::Variable variable;

		// In which block?
		code::Block block;
	};

}
