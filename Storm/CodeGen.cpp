#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"

namespace storm {

	using code::Variable;
	using code::Block;
	using code::Frame;

	Variable variable(Frame &frame, Block block, const Value &v) {
		code::FreeOpt opt = code::freeOnBoth;
		code::Value dtor = v.destructor();
		if (v != Value())
			if (v.type->flags & typeValue)
				opt = opt | code::freePtr;

		return frame.createVariable(block, v.size(), dtor, opt);
	}


	GenResult::GenResult() : type(), variable(Variable::invalid), block(Block::invalid) {}

	GenResult::GenResult(const Value &t, Block block) : type(t), variable(Variable::invalid), block(block) {}

	GenResult::GenResult(const Value &t, Variable var) : type(t), variable(var), block(code::Block::invalid) {}

	code::Variable GenResult::location(const GenState &state) {
		assert(needed(), "Trying to get the location of an unneeded result. use safeLocation instead.");

		if (variable == Variable::invalid) {
			if (block == Block::invalid) {
				variable = storm::variable(state, type);
			} else {
				variable = storm::variable(state.frame, block, type);
			}
		}
		return variable;
	}

	code::Variable GenResult::safeLocation(const GenState &s, const Value &t) {
		if (needed())
			return location(s);
		else if (variable == Variable::invalid)
			return variable = storm::variable(s, t);
		else
			return variable;
	}

	bool GenResult::suggest(const GenState &s, code::Value v) {
		if (v.type() == code::Value::tVariable)
			return suggest(s, v.variable());
		return false;
	}

	bool GenResult::suggest(const GenState &s, Variable v) {
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

	code::Variable createBasicTypeInfo(const GenState &to, const Value &v) {
		using namespace code;

		BasicTypeInfo typeInfo = v.typeInfo();

		Size s = Size::sNat * 2;
		assert(s.current() == sizeof(typeInfo), L"Please check the declaration of BasicTypeInfo.");

		Variable r = to.frame.createVariable(to.block, s);
		to.to << lea(ptrA, r);
		to.to << mov(intRel(ptrA), natConst(typeInfo.size));
		to.to << mov(intRel(ptrA, Offset::sNat), natConst(typeInfo.kind));

		return r;
	}

}
