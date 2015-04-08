#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"
#include "Function.h"

namespace storm {

	using code::Variable;
	using code::Block;
	using code::Part;
	using code::Frame;

	/**
	 * Code generation.
	 */

	CodeGen::CodeGen(RunOn runOn) :
		l(CREATE(wrap::Listing, this)),
		data(CREATE(CodeData, this)),
		runOn(runOn),
		block(l->root()),
		to(l->v),
		frame(to.frame) {}

	CodeGen::CodeGen(Par<CodeGen> o) :
		l(o->l),
		data(o->data),
		runOn(o->runOn),
		block(o->block),
		to(o->to),
		frame(o->frame) {}

	CodeGen *CodeGen::child(wrap::Block block) {
		CodeGen *o = CREATE(CodeGen, this, this);
		o->block = block;
		return o;
	}

	/**
	 * Variable info.
	 */

	VarInfo::VarInfo() : v(Variable::invalid), needsPart(false) {}
	VarInfo::VarInfo(wrap::Variable v) : v(v), needsPart(false) {}
	VarInfo::VarInfo(wrap::Variable v, bool p) : v(v), needsPart(p) {}

	void VarInfo::created(Par<CodeGen> to) {
		if (!needsPart)
			return;

		Part root = to->frame.parent(v.v);
		assert(to->frame.first(root) == to->block.v,
			L"The variable " + ::toS(code::Value(v.v)) + L" was already created, or in wrong block: "
			+ ::toS(code::Value(to->block.v)) + L"." + ::toS(to->to));

		Part created = to->frame.createPart(root);
		to->frame.delay(v.v, created);
		to->to << begin(created);
	}

	/**
	 * Create a variable.
	 */

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


	/**
	 * CodeResult.
	 */

	CodeResult::CodeResult() : type(), block(Block::invalid) {}
	CodeResult::CodeResult(const Value &t, wrap::Block block) : type(t), block(block.v) {}
	CodeResult::CodeResult(const Value &t, VarInfo v) : type(t), variable(v), block(Block::invalid) {}

	VarInfo CodeResult::location(Par<CodeGen> s) {
		assert(needed(), "Trying to get the location of an unneeded result. Use 'safeLocation' instead.");

		if (variable.var() == Variable::invalid) {
			if (block == Block::invalid) {
				variable = storm::variable(s, type);
			} else {
				variable = storm::variable(s->frame, block, type);
			}
		}

		Frame &f = s->frame;
		if (variable.needsPart && f.first(f.parent(variable.var())) != state.block)
			// We need to delay the part transition until we have exited the current block!
			return VarInfo(variable.var(), false);
		return variable;
	}

	VarInfo CodeResult::safeLocation(Par<CodeGen> s) {
		if (needed())
			return location(s);
		else if (variable.var() == Variable::invalid)
			return variable = storm::variable(s, t);
		else
			return variable;
	}

	Bool CodeResult::suggest(Par<CodeGen> s, wrap::Variable v) {
		Frame &f = s->frame;

		if (variable.var() != Variable::invalid)
			return false;

		// TODO? Cases that hit here could maybe be optimized somehow!
		// this is common with the return value, which will almost always
		// have to get its lifetime extended a bit. Maybe implement the
		// possibility to move variables to a more outer scope?
		if (block != Block::invalid && !f.accessible(f.first(block), v))
			return false;

		variable = VarInfo(v);
		return true;
	}

	Bool CodeResult::suggest(Par<CodeGen> s, wrap::Operand v) {
		if (v.v.type() == code::Value::tVariable)
			return suggest(s, v.variable());
		return false;
	}


	/**
	 * OLD CODE FROM HERE \/
	 */

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
