#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"
#include "Function.h"

namespace storm {

	using code::Variable;
	using code::Block;
	using code::Part;
	using code::Frame;

	VarInfo::VarInfo() : var(Variable::invalid), needsPart(false) {}

	VarInfo::VarInfo(const Variable &v) : var(v), needsPart(false) {}

	VarInfo::VarInfo(Variable v, bool p) : var(v), needsPart(p) {}

	void VarInfo::created(const GenState &to) {
		if (!needsPart)
			return;

		Part root = to.frame.parent(var);
		assert(to.frame.first(root) == to.block,
			L"The variable " + ::toS(code::Value(var)) + L" was already created, or in wrong block: "
			+ ::toS(code::Value(to.block)) + L"." + ::toS(to.to));

		Part created = to.frame.createPart(root);
		to.frame.delay(var, created);

		to.to << begin(created);
	}

	VarInfo variable(Frame &frame, Block block, const Value &v) {
		code::FreeOpt opt = code::freeOnBoth;
		code::Value dtor = v.destructor();
		bool needsPart = false;

		if (v.isValue()) {
			opt = opt | code::freePtr;
			needsPart = true;
		}

		return VarInfo(frame.createVariable(block, v.size(), dtor, opt), needsPart);
	}


	GenResult::GenResult() : type(), block(Block::invalid) {}

	GenResult::GenResult(const Value &t, Block block) : type(t), block(block) {}

	GenResult::GenResult(const Value &t, VarInfo var) : type(t), variable(var), block(code::Block::invalid) {}

	VarInfo GenResult::location(const GenState &state) {
		assert(needed(), "Trying to get the location of an unneeded result. use safeLocation instead.");

		if (variable.var == Variable::invalid) {
			if (block == Block::invalid) {
				variable = storm::variable(state, type);
			} else {
				variable = storm::variable(state.frame, block, type);
			}
		}

		Frame &f = state.frame;
		if (variable.needsPart && f.first(f.parent(variable.var)) != state.block)
			// We need to delay the part transition until we have exited the current block!
			return VarInfo(variable.var, false);
		return variable;
	}

	VarInfo GenResult::safeLocation(const GenState &s, const Value &t) {
		if (needed())
			return location(s);
		else if (variable.var == Variable::invalid)
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

		Frame &f = s.frame;

		if (variable.var == Variable::invalid) {
			if (block != Block::invalid) {
				if (!f.accessible(f.first(block), v)) {
					// TODO? Cases that hit here could maybe be optimized somehow!
					// this is common with the return value, which will almost always
					// have to get its lifetime extended a bit. Maybe implement the
					// possibility to move variables to a more outer scope?
					return false;
				}
			}
			variable = VarInfo(v);
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

	void allocObject(code::Listing &l, code::Block b, Function *ctor, vector<code::Value> params, code::Variable to) {
		using namespace code;

		Type *type = ctor->params[0].type;
		Engine &e = ctor->engine();
		assert(type->flags & typeClass, L"Must allocate class objects.");

		Block sub = l.frame.createChild(l.frame.last(b));
		Variable rawMem = l.frame.createPtrVar(sub, e.fnRefs.freeRef, freeOnException);

		l << begin(sub);
		l << fnParam(type->typeRef);
		l << fnCall(e.fnRefs.allocRef, Size::sPtr);
		l << mov(rawMem, ptrA);

		l << fnParam(ptrA);
		for (nat i = 0; i < params.size(); i++)
			l << fnParam(params[i]);
		l << fnCall(ctor->ref(), Size());

		l << mov(to, rawMem);
		l << end(sub);
	}

	void allocObject(const GenState &s, Function *ctor, vector<code::Value> params, code::Variable to) {
		using namespace code;

		Type *type = ctor->params[0].type;
		Engine &e = ctor->engine();
		assert(type->flags & typeClass, L"Must allocate class objects.");

		Block b = s.frame.createChild(s.frame.last(s.block));
		Variable rawMem = s.frame.createPtrVar(b, e.fnRefs.freeRef, freeOnException);

		s.to << begin(b);
		s.to << fnParam(type->typeRef);
		s.to << fnCall(e.fnRefs.allocRef, Size::sPtr);
		s.to << mov(rawMem, ptrA);

		GenResult r;
		params.insert(params.begin(), code::Value(ptrA));
		ctor->localCall(s.child(b), params, r, false);

		s.to << mov(to, rawMem);
		s.to << end(b);
	}

}
