#include "stdafx.h"
#include "CodeGen.h"

namespace storm {

	GenResult::GenResult() : needed(false), variable(code::Variable::invalid), block(code::Block::invalid) {}

	GenResult::GenResult(code::Block block) : needed(true), variable(code::Variable::invalid), block(block) {}

	GenResult::GenResult(code::Variable var) : needed(true), variable(var), block(code::Block::invalid) {}

	code::Variable GenResult::location(const GenState &state, const Value &t) {
		if (variable == code::Variable::invalid) {
			if (block == code::Block::invalid) {
				variable = storm::variable(state, t);
			} else {
				variable = storm::variable(state.frame, block, t);
			}
		}
		return variable;
	}

	bool GenResult::suggest(code::Variable v) {
		if (variable == code::Variable::invalid) {
			variable = v;
			return true;
		} else {
			return false;
		}
	}

}
