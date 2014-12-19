#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"

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

	bool GenResult::suggest(const GenState &s, code::Variable v) {
		using namespace code;

		if (variable == Variable::invalid) {
			if (block != Block::invalid) {
				if (!s.frame.outlives(v, block)) {
					// TODO? Cases that hit here could maybe be optimized somehow!
					// this is common with the return value, which will almost always
					// have to get its lifetime extended a bit. Maybe implement the
					// possibility to move variables to a more outer scope?
					return false;
				}
			}
			variable = v;
			return true;
		} else {
			return false;
		}
	}

	void cast(const GenState &state, GenResult &r, code::Value &src, const Value &from, const Value &to) {
		using namespace code;

		if (!to.canStore(from))
			throw InternalError(L"Can not cast " + toS(from) + L" to " + toS(to) + L"!");

		// Do we actually need to do something?
		if (from.ref == to.ref) {
			if (src.type() != code::Value::tVariable || !r.suggest(state, src.variable()))
				state.to << mov(r.location(state, to), src);
			return;
		}

		// Cast to a reference?
		if (!from.ref) {
			Variable dest = r.location(state, to);
			state.to << lea(dest, src);
			return;
		}

		// Cast from a reference.
		if (from.ref) {
			Variable dest = r.location(state, to);
			state.to << mov(ptrA, src);
			state.to << mov(dest, xRel(to.size(), ptrA, 0));
			return;
		}

		// Failed!
		assert(false);
		throw InternalError(L"Cast failed. Do not know how to cast " + toS(from) + L" to " + toS(to) + L"!");
	}
}
