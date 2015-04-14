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

	void CodeGen::output(wostream &to) const {
		to << L"Run on: " << runOn << endl;
		to << L"Current block: " << code::Value(block.v) << endl;
		to << l;
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

	CodeResult::CodeResult() : t(), block(Block::invalid) {}
	CodeResult::CodeResult(const Value &t, wrap::Block block) : t(t), block(block.v) {}
	CodeResult::CodeResult(const Value &t, VarInfo v) : t(t), variable(v), block(Block::invalid) {}

	VarInfo CodeResult::location(Par<CodeGen> s) {
		assert(needed(), "Trying to get the location of an unneeded result. Use 'safeLocation' instead.");

		if (variable.var() == Variable::invalid) {
			if (block == Block::invalid) {
				variable = storm::variable(s, t);
			} else {
				variable = storm::variable(s->frame, block, t);
			}
		}

		Frame &f = s->frame;
		if (variable.needsPart && f.first(f.parent(variable.var())) != s->block.v)
			// We need to delay the part transition until we have exited the current block!
			return VarInfo(variable.var(), false);
		return variable;
	}

	VarInfo CodeResult::safeLocation(Par<CodeGen> s, const Value &t) {
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
		if (block != Block::invalid && !f.accessible(f.first(block), v.v))
			return false;

		variable = VarInfo(v);
		return true;
	}

	Bool CodeResult::suggest(Par<CodeGen> s, wrap::Operand v) {
		if (v.v.type() == code::Value::tVariable)
			return suggest(s, v.v.variable());
		return false;
	}


	code::Variable createBasicTypeInfo(Par<CodeGen> to, const Value &v) {
		using namespace code;

		BasicTypeInfo typeInfo = v.typeInfo();

		Size s = Size::sNat * 2;
		assert(s.current() == sizeof(typeInfo), L"Please check the declaration of BasicTypeInfo.");

		Variable r = to->frame.createVariable(to->block.v, s);
		to->to << lea(ptrA, r);
		to->to << mov(intRel(ptrA), natConst(typeInfo.size));
		to->to << mov(intRel(ptrA, Offset::sNat), natConst(typeInfo.kind));

		return r;
	}

	void allocObject(code::Listing &l, code::Block b, Par<Function> ctor, vector<code::Value> params, code::Variable to) {
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

	void allocObject(Par<CodeGen> s, Par<Function> ctor, vector<code::Value> params, code::Variable to) {
		using namespace code;

		Type *type = ctor->params[0].type;
		Engine &e = ctor->engine();
		assert(type->flags & typeClass, L"Must allocate class objects.");

		Block b = s->frame.createChild(s->frame.last(s->block.v));
		Variable rawMem = s->frame.createPtrVar(b, e.fnRefs.freeRef, freeOnException);

		s->to << begin(b);
		s->to << fnParam(type->typeRef);
		s->to << fnCall(e.fnRefs.allocRef, Size::sPtr);
		s->to << mov(rawMem, ptrA);

		Auto<CodeResult> r = CREATE(CodeResult, s);
		params.insert(params.begin(), code::Value(ptrA));
		ctor->localCall(steal(s->child(b)), params, r, false);

		s->to << mov(to, rawMem);
		s->to << end(b);
	}

	wrap::Variable createFnParams(Par<CodeGen> s, wrap::Operand memory) {
		using namespace code;

		Engine &e = s->engine();
		Variable v = s->frame.createVariable(s->block.v,
											FnParams::classSize(),
											e.fnRefs.fnParamsDtor,
											freeOnBoth | freePtr);
		// Call the ctor!
		s->to << lea(ptrC, v);
		s->to << fnParam(ptrC);
		s->to << fnParam(memory.v);
		s->to << fnCall(e.fnRefs.fnParamsCtor, Size());

		return v;
	}

	wrap::Variable createFnParams(Par<CodeGen> s, Nat params) {
		using namespace code;
		// Allocate space for the parameters on the stack.
		Size total = FnParams::paramSize() * params;
		Variable v = s->frame.createVariable(s->block.v, total);
		s->to << lea(ptrA, v);

		return createFnParams(s, code::Value(ptrA));
	}

	void STORM_FN addFnParam(Par<CodeGen> s, wrap::Variable fnParams, const Value &type, const wrap::Operand &v) {
		using namespace code;
		Engine &e = s->engine();

		if (type.isClass()) {
			s->to << lea(ptrC, fnParams.v);
			s->to << lea(ptrA, v.v);
			s->to << fnParam(ptrC);
			s->to << fnParam(e.fnRefs.copyRefPtr);
			s->to << fnParam(e.fnRefs.releasePtr);
			s->to << fnParam(natConst(type.size()));
			s->to << fnParam(ptrA);
			s->to << fnCall(e.fnRefs.fnParamsAdd, Size());
		} else if (type.ref || type.isBuiltIn()) {
			s->to << lea(ptrC, fnParams.v);
			s->to << fnParam(ptrC);
			s->to << fnParam(intPtrConst(0));
			s->to << fnParam(intPtrConst(0));
			s->to << fnParam(natConst(type.size()));
			s->to << fnParam(v.v);
			s->to << fnCall(e.fnRefs.fnParamsAdd, Size());
		} else {
			// Value.
			code::Value dtor = type.destructor();
			if (dtor.empty())
				dtor = intPtrConst(0);
			s->to << lea(ptrC, fnParams.v);
			s->to << lea(ptrA, v.v);
			s->to << fnParam(ptrC);
			s->to << fnParam(type.copyCtor());
			s->to << fnParam(dtor);
			s->to << fnParam(natConst(type.size()));
			s->to << fnParam(ptrA);
			s->to << fnCall(e.fnRefs.fnParamsAdd, Size());
		}
	}

	// Find 'std:clone' for the given type.
	wrap::Operand stdCloneFn(const Value &type) {
		if (type == Value())
			return wrap::Operand();

		Engine &e = type.type->engine;
		Auto<Name> name = CREATE(Name, e);
		name->add(L"core");
		name->add(L"clone", valList(1, type));
		Function *f = as<Function>(e.scope()->find(name));
		if (!f)
			throw InternalError(L"Could not find std.clone(" + ::toS(type) + L").");
		return code::Value(f->ref());
	}

	void STORM_FN addFnParamCopy(Par<CodeGen> s, wrap::Variable fnParams, const Value &type, const wrap::Operand &v) {
		using namespace code;
		Engine &e = s->engine();

		if (type.isClass()) {
			Variable clone = s->frame.createPtrVar(s->block.v, e.fnRefs.release);
			s->to << fnParam(v.v);
			s->to << fnCall(stdCloneFn(type).v, Size::sPtr);
			s->to << mov(clone, ptrA);

			// Regular parameter add.
			addFnParam(s, fnParams, type, wrap::Variable(clone));
		} else if (type.ref || type.isBuiltIn()) {
			if (type.ref) {
				TODO(L"Should we really allow pretending to deep-clone references?");
			}

			// Reuse the plain one.
			addFnParam(s, fnParams, type, v);
		} else {
			// We can use stdClone as the copy ctor in this case.
			code::Value dtor = type.destructor();
			if (dtor.empty())
				dtor = intPtrConst(0);
			s->to << lea(ptrC, fnParams.v);
			s->to << lea(ptrA, v.v);
			s->to << fnParam(ptrC);
			s->to << fnParam(stdCloneFn(type).v);
			s->to << fnParam(dtor);
			s->to << fnParam(natConst(type.size()));
			s->to << fnParam(ptrA);
			s->to << fnCall(e.fnRefs.fnParamsAdd, Size());
		}
	}

	void STORM_FN addFnParamPlain(Par<CodeGen> s, wrap::Variable fnParams, const wrap::Operand &v) {
		using namespace code;
		Engine &e = s->engine();

		assert(v.size() == Size::sPtr);

		s->to << lea(ptrC, fnParams.v);
		s->to << fnParam(ptrC);
		s->to << fnParam(intPtrConst(0));
		s->to << fnParam(intPtrConst(0));
		s->to << fnParam(natConst(Size::sPtr));
		s->to << fnParam(v.v);
		s->to << fnCall(e.fnRefs.fnParamsAdd, Size());
	}

}
