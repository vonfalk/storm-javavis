#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"
#include "Engine.h"
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

	CodeGen *CodeGen::child() {
		code::Block b = to->createBlock(to->last(block));
		return child(b);
	}

	void CodeGen::deepCopy(CloneEnv *env) {
		clone(to, env);
	}

	code::Var CodeGen::createParam(Value type) {
		if (type.isValue()) {
			return to->createParam(type.valTypeParam(), type.destructor(), code::freeOnBoth | code::freePtr);
		} else {
			return to->createParam(type.valTypeParam());
		}
	}

	VarInfo CodeGen::createVar(Value type) {
		return createVar(type, block);
	}

	VarInfo CodeGen::createVar(Value type, code::Block in) {
		code::FreeOpt opt = code::freeOnBoth;
		code::Operand dtor;
		bool needsPart = type.isValue() && !type.ref;

		if (needsPart) {
			dtor = type.destructor();
			opt = opt | code::freePtr;
		}

		return VarInfo(to->createVar(in, type.size(), dtor, opt), needsPart);
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

	void CodeGen::returnValue(code::Var value) {
		using namespace code;

		if (res == Value()) {
			*to << epilog();
			*to << ret(valVoid());
		} else if (res.returnInReg()) {
			*to << mov(asSize(ptrA, res.size()), value);
			*to << epilog();
			*to << ret(res.valTypeRet());
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

	VarInfo::VarInfo(code::Var v) : v(v), needsPart(false) {}

	VarInfo::VarInfo(code::Var v, Bool needsPart) : v(v), needsPart(needsPart) {}

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

	CodeResult::CodeResult(Value type, code::Var var) : variable(var), t(type) {}

	CodeResult::CodeResult(Value type, VarInfo var) : variable(var), t(type) {}

	VarInfo CodeResult::location(CodeGen *s) {
		assert(needed(), L"Trying to get the location of an unneeded result. Use 'safeLocation' instead.");

		if (variable.v == code::Var()) {
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
		else if (variable.v == code::Var())
			// Generate a temporary variable. Do not save it, it will mess up cases like 'if', where
			// two different branches may end up in different blocks!
			return s->createVar(type);
		else
			return variable;
	}

	Bool CodeResult::suggest(CodeGen *s, code::Var v) {
		code::Listing *l = s->to;

		if (variable.v != code::Var())
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
			return suggest(s, v.var());
		return false;
	}

	Value CodeResult::type() const {
		return t;
	}

	Bool CodeResult::needed() const {
		return t != Value();
	}

	code::Var createBasicTypeInfo(CodeGen *to, Value v) {
		using namespace code;

		BasicTypeInfo typeInfo = v.typeInfo();

		Size s = Size::sNat * 2;
		assert(s.current() == sizeof(typeInfo), L"Please check the declaration of BasicTypeInfo.");

		Var r = to->to->createVar(to->block, s);
		*to->to << lea(ptrA, r);
		*to->to << mov(intRel(ptrA, Offset()), natConst(typeInfo.size));
		*to->to << mov(intRel(ptrA, Offset::sNat), natConst(typeInfo.kind));

		return r;
	}

	static code::Size fnParamsSize() {
		Size s = Size::sPtr;
		s += Size::sNat;
		s += Size::sNat;
		assert(s.current() == sizeof(os::FnParams), L"Please update the size here!");
		return s;
	}

	static code::Size fnParamSize() {
		Size s = Size::sPtr * 3;
		s += Size::sNat;
		assert(s.current() == sizeof(os::FnParams::Param), L"Please update the size here!");
		return s;
	}

	code::Var createFnParams(CodeGen *s, code::Operand memory) {
		using namespace code;

		Engine &e = s->engine();
		Var v = s->to->createVar(s->block,
								fnParamsSize(),
								e.ref(Engine::rFnParamsDtor),
								freeOnBoth | freePtr);
		// Call the ctor!
		*s->to << lea(ptrC, v);
		*s->to << fnParam(ptrC);
		*s->to << fnParam(memory);
		*s->to << fnCall(e.ref(Engine::rFnParamsCtor), valVoid());

		return v;
	}

	code::Var createFnParams(CodeGen *s, Nat params) {
		using namespace code;
		// Allocate space for the parameters on the stack.
		Size total = fnParamSize() * params;
		Var v = s->to->createVar(s->block, total);
		*s->to << lea(ptrA, v);

		return createFnParams(s, code::Operand(ptrA));
	}

	void addFnParam(CodeGen *s, code::Var fnParams, Value type,	code::Operand v) {
		using namespace code;
		Engine &e = s->engine();

		if (type.isHeapObj() || type.ref) {
			*s->to << lea(ptrC, fnParams);
			*s->to << fnParam(ptrC);
			*s->to << fnParam(ptrConst(Offset()));
			*s->to << fnParam(ptrConst(Offset()));
			*s->to << fnParam(ptrConst(type.size()));
			*s->to << fnParam(byteConst(0));
			*s->to << fnParam(v);
			*s->to << fnCall(e.ref(Engine::rFnParamsAdd), valVoid());
		} else if (type.isBuiltIn()) {
			*s->to << lea(ptrC, fnParams);
			*s->to << lea(ptrA, v);
			*s->to << fnParam(ptrC);
			*s->to << fnParam(ptrConst(Offset()));
			*s->to << fnParam(ptrConst(Offset()));
			*s->to << fnParam(ptrConst(type.size()));
			*s->to << fnParam(byteConst(type.isFloat() ? 1 : 0));
			*s->to << fnParam(ptrA);
			*s->to << fnCall(e.ref(Engine::rFnParamsAdd), valVoid());
		} else {
			// Value.
			code::Operand dtor = type.destructor();
			if (dtor.empty())
				dtor = ptrConst(Offset());
			*s->to << lea(ptrC, fnParams);
			*s->to << lea(ptrA, v);
			*s->to << fnParam(ptrC);
			*s->to << fnParam(type.copyCtor());
			*s->to << fnParam(dtor);
			*s->to << fnParam(ptrConst(type.size()));
			*s->to << fnParam(byteConst(0));
			*s->to << fnParam(ptrA);
			*s->to << fnCall(e.ref(Engine::rFnParamsAdd), valVoid());
		}
	}

	void addFnParamCopy(CodeGen *s, code::Var fnParams, Value type, code::Operand v) {
		TODO(L"Implement me properly!");
		addFnParam(s, fnParams, type, v);

		// using namespace code;
		// Engine &e = s->engine();

		// if (type.isHeapObj()) {
		// 	Variable clone = s->frame.createPtrVar(s->block.v, e.fnRefs.release);
		// 	s->to << fnParam(v.v);
		// 	s->to << fnCall(stdCloneFn(type).v, retPtr());
		// 	s->to << mov(clone, ptrA);

		// 	// Regular parameter add.
		// 	addFnParam(s, fnParams, type, code::Var(clone), thunk);
		// } else if (type.ref || type.isBuiltIn()) {
		// 	if (type.ref) {
		// 		TODO(L"Should we really allow pretending to deep-clone references?");
		// 	}

		// 	// Reuse the plain one.
		// 	addFnParam(s, fnParams, type, v, thunk);
		// } else {
		// 	// We can use stdClone as the copy ctor in this case.
		// 	code::Operand dtor = type.destructor();
		// 	if (dtor.empty())
		// 		dtor = intPtrConst(0);
		// 	s->to << lea(ptrC, fnParams.v);
		// 	s->to << lea(ptrA, v.v);
		// 	s->to << fnParam(ptrC);
		// 	s->to << fnParam(stdCloneFn(type).v);
		// 	s->to << fnParam(dtor);
		// 	s->to << fnParam(natConst(type->count()));
		// 	s->to << fnParam(byteConst(0));
		// 	s->to << fnParam(ptrA);
		// 	s->to << fnCall(e.fnRefs.fnParamsAdd, retVoid());
		// }
	}

	static void allocNormalObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to) {
		using namespace code;

		Type *type = ctor->params->at(0).type;
		Engine &e = ctor->engine();

		*s->to << fnParam(type->typeRef());
		*s->to << fnCall(e.ref(Engine::rAlloc), valPtr());
		*s->to << mov(to, ptrA);

		CodeResult *r = new (s) CodeResult();
		params = new (s) Array<code::Operand>(*params);
		params->insert(0, to);
		ctor->autoCall(s, params, r);
	}

	static void allocRawObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to) {
		using namespace code;

		Type *type = ctor->params->at(0).type;
		Engine &e = ctor->engine();

		*s->to << lea(ptrA, to);

		CodeResult *r = new (s) CodeResult();
		params = new (s) Array<code::Operand>(*params);
		params->insert(0, to);
		ctor->autoCall(s, params, r);
	}

	void allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to) {
		Value t = ctor->params->at(0);
		assert(t.isHeapObj(), L"Must allocate a class type!");

		if (t.type->typeFlags & typeRawPtr)
			allocRawObject(s, ctor, params, to);
		else
			allocNormalObject(s, ctor, params, to);
	}

	code::Var allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params) {
		code::Var r = s->to->createVar(s->block, Size::sPtr);
		allocObject(s, ctor, params, r);
		return r;
	}

}
