#pragma once
#include "Code/Listing.h"
#include "Value.h"

namespace storm {

	/**
	 * Describes the state of code generation.
	 */
	struct GenState {
		code::Listing &to;
		code::Frame &frame;
		code::Block block;
	};

	// Helper to create a variable from a Value (this is common).
	inline code::Variable variable(code::Frame &frame, code::Block block, const Value &v) {
		return frame.createVariable(block, v.size(), v.destructor());
	}

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

		// A result is needed in 'block'.
		GenResult(code::Block block);

		// Indicate that the result should be stored in 'var'.
		GenResult(code::Variable var);

		// Get the location of the result (it will be created if it does not exist).
		code::Variable location(const GenState &s, const Value &type);

		// Suggest a location. Returns true if it is used. Otherwise, store the value in whatever
		// location is returned by 'location'.
		bool suggest(code::Variable v);

		// Do we need a result?
		const bool needed;

	private:
		// Stored variable.
		code::Variable variable;

		// In which block?
		code::Block block;
	};

}
