#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"
#include "Core/CloneEnv.h"

namespace storm {

	CodeGen::CodeGen(RunOn thread) : runOn(thread) {
		to = new (this) code::Listing();
		block = to->root();
	}

	CodeGen::CodeGen(RunOn thread, code::Listing *to) : runOn(thread), to(to), block(to->root()) {}

	CodeGen::CodeGen(RunOn thread, code::Listing *to, code::Block block) : runOn(thread), to(to), block(block) {}

	CodeGen::CodeGen(CodeGen *me, code::Block b) :
		runOn(me->runOn), to(me->to), block(b), res(me->res), resParam(me->resParam) {}

	CodeGen *CodeGen::child(code::Block b) {
		return new (this) CodeGen(this, b);
	}

	void CodeGen::deepCopy(CloneEnv *env) {
		clone(to, env);
	}

	code::Variable CodeGen::createParam(Value type) {
		if (type.isValue()) {
			return to->createParam(type.valType(), type.destructor(), code::freeOnBoth | code::freePtr);
		} else {
			return to->createParam(type.valType());
		}
	}

	VarInfo CodeGen::createVar(Value type) {
		return createVar(type, block);
	}

	VarInfo CodeGen::createVar(Value type, code::Block in) {
		code::FreeOpt opt = code::freeOnBoth;
		code::Operand dtor = type.destructor();
		bool needsPart = type.isValue();

		if (needsPart)
			opt = opt | code::freePtr;

		return VarInfo(to->createVar(block, type.size(), dtor, opt), needsPart);
	}


	void CodeGen::result(Value type, Bool isMember) {
		if (res != Value())
			throw InternalError(L"Trying to re-set the return type of CodeGen.");

		res = type;
		if (type.isValue()) {
			// We need a return value parameter as either the first or the second parameter!
			resParam = to->createParam(code::valPtr());
			to->moveParam(resParam, isMember ? 1 : 0);
		}
	}

	Value CodeGen::result() const {
		return res;
	}

	void CodeGen::returnValue(code::Variable value) {
		using namespace code;

		if (res == Value()) {
			*to << epilog();
			*to << ret(valVoid());
		} else if (res.returnInReg()) {
			*to << mov(asSize(ptrA, res.size()), value);
			*to << epilog();
			*to << ret(res.valType());
		} else {
			*to << lea(ptrA, ptrRel(value, Offset()));
			*to << fnParam(resParam);
			*to << fnParam(ptrA);
			*to << fnCall(res.copyCtor(), valPtr());

			// We need to provide the address of the return value as our result. The copy ctor does
			// not neccessarily return an address to the created value. This is important in some
			// optimized builds, where the compiler assumes ptrA contains the address of the
			// returned value. This is usually not the case in unoptimized builds.
			*to << mov(ptrA, resParam);
			*to << epilog();
			*to << ret(valPtr());
		}
	}


	/**
	 * VarInfo.
	 */

	VarInfo::VarInfo() : v(), needsPart(false) {}

	VarInfo::VarInfo(code::Variable v) : v(v), needsPart(false) {}

	VarInfo::VarInfo(code::Variable v, Bool needsPart) : v(v), needsPart(needsPart) {}

	void VarInfo::created(CodeGen *gen) {
		using namespace code;

		if (!needsPart)
			return;

		Listing *to = gen->to;
		Part root = to->parent(v);
		assert(to->first(root) == gen->block,
			L"The variable " + ::toS(v) + L" was already created, or in the wrong block: "
			+ ::toS(gen->block) + L".");

		Part created = to->createPart(root);
		to->delay(v, created);
		*to << begin(created);
	}


	/**
	 * CodeResult.
	 */

	CodeResult::CodeResult() {}

	CodeResult::CodeResult(Value type, code::Block block) : block(block), t(type) {}

	CodeResult::CodeResult(Value type, code::Variable var) : variable(var), t(type) {}

	VarInfo CodeResult::location(CodeGen *s) {
		assert(needed(), L"Trying to get the location of an unneeded result. Use 'safeLocation' instead.");

		if (variable.v == code::Variable()) {
			if (block == code::Block()) {
				variable = s->createVar(t);
			} else {
				variable = s->createVar(t, block);
			}
		}

		code::Listing *l = s->to;
		if (variable.needsPart && l->first(l->parent(variable.v)) != s->block)
			// We need to delay the part transition until we have exited the current block!
			return VarInfo(variable.v, false);
		else
			return variable;
	}

	VarInfo CodeResult::safeLocation(CodeGen *s, Value type) {
		if (needed())
			return location(s);
		else if (variable.v == code::Variable())
			// Generate a temporary variable. Do not save it, it will mess up cases like 'if', where
			// two different branches may end up in different blocks!
			return s->createVar(t);
		else
			return variable;
	}

	Bool CodeResult::suggest(CodeGen *s, code::Variable v) {
		code::Listing *l = s->to;

		if (variable.v != code::Variable())
			return false;

		// TODO: Cases that hit here could maybe be optimized somehow. This is common with the
		// return value, which will almost always have to get its lifetime extended a bit. Maybe
		// implement the possibility to move variables to a more outer scope?
		if (block != code::Block() && !l->accessible(v, l->first(block)))
			return false;

		variable = VarInfo(v);
		return true;
	}

	Bool CodeResult::suggest(CodeGen *s, code::Operand v) {
		if (v.type() == code::opVariable)
			return suggest(s, v.variable());
		return false;
	}

	Value CodeResult::type() const {
		return t;
	}

	Bool CodeResult::needed() const {
		return t != Value();
	}

}
