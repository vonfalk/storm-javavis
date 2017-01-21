#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"
#include "Engine.h"
#include "Core/CloneEnv.h"
#include "Lib/Clone.h"

namespace storm {

	CodeGen::CodeGen(RunOn thread) : runOn(thread) {
		l = new (this) code::Listing();
		block = l->root();
	}

	CodeGen::CodeGen(RunOn thread, code::Listing *to) : runOn(thread), l(to), block(to->root()) {}

	CodeGen::CodeGen(RunOn thread, code::Listing *to, code::Block block) : runOn(thread), l(to), block(block) {}

	CodeGen::CodeGen(CodeGen *me, code::Block b) :
		runOn(me->runOn), l(me->l), block(b), res(me->res), resParam(me->resParam) {}

	CodeGen *CodeGen::child(code::Block b) {
		return new (this) CodeGen(this, b);
	}

	CodeGen *CodeGen::child() {
		code::Block b = l->createBlock(l->last(block));
		return child(b);
	}

	void CodeGen::deepCopy(CloneEnv *env) {
		clone(l, env);
	}

	code::Var CodeGen::createParam(Value type) {
		if (type.isValue()) {
			return l->createParam(type.valTypeParam(), type.destructor(), code::freeOnBoth | code::freePtr);
		} else {
			return l->createParam(type.valTypeParam());
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

		return VarInfo(l->createVar(in, type.size(), dtor, opt), needsPart);
	}

	void CodeGen::result(Value type, Bool isMember) {
		if (res != Value())
			throw InternalError(L"Trying to re-set the return type of CodeGen.");

		res = type;
		if (type.isValue()) {
			// We need a return value parameter as either the first or the second parameter!
			resParam = l->createParam(code::valPtr());
			l->moveParam(resParam, isMember ? 1 : 0);
		}
	}

	Value CodeGen::result() const {
		return res;
	}

	void CodeGen::returnValue(code::Var value) {
		using namespace code;

		if (res == Value()) {
			*l << epilog();
			*l << ret(valVoid());
		} else if (res.returnInReg()) {
			*l << mov(asSize(ptrA, res.size()), value);
			*l << epilog();
			*l << ret(res.valTypeRet());
		} else {
			*l << lea(ptrA, ptrRel(value, Offset()));
			*l << fnParam(resParam);
			*l << fnParam(ptrA);
			*l << fnCall(res.copyCtor(), valPtr());

			// We need to provide the address of the return value as our result. The copy ctor does
			// not neccessarily return an address to the created value. This is important in some
			// optimized builds, where the compiler assumes ptrA contains the address of the
			// returned value. This is usually not the case in unoptimized builds.
			*l << mov(ptrA, resParam);
			*l << epilog();
			*l << ret(valPtr());
		}
	}

	void CodeGen::toS(StrBuf *to) const {
		*to << L"Running on: " << runOn << L"\n";
		*to << L"Current block: " << block << L"\n";
		*to << l;
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

		Listing *to = gen->l;
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

		code::Listing *l = s->l;
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
		code::Listing *l = s->l;

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

		Var r = to->l->createVar(to->block, s);
		*to->l << lea(ptrA, r);
		*to->l << mov(intRel(ptrA, Offset()), natConst(typeInfo.size));
		*to->l << mov(intRel(ptrA, Offset::sNat), natConst(typeInfo.kind));

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
		Var v = s->l->createVar(s->block,
								fnParamsSize(),
								e.ref(Engine::rFnParamsDtor),
								freeOnBoth | freePtr);
		// Call the ctor!
		*s->l << lea(ptrC, v);
		*s->l << fnParam(ptrC);
		*s->l << fnParam(memory);
		*s->l << fnCall(e.ref(Engine::rFnParamsCtor), valVoid());

		return v;
	}

	code::Var createFnParams(CodeGen *s, Nat params) {
		using namespace code;
		// Allocate space for the parameters on the stack.
		Size total = fnParamSize() * params;
		Var v = s->l->createVar(s->block, total);
		*s->l << lea(ptrA, v);

		return createFnParams(s, code::Operand(ptrA));
	}

	void addFnParam(CodeGen *s, code::Var fnParams, Value type,	code::Operand v) {
		using namespace code;
		Engine &e = s->engine();

		if (type.isHeapObj() || type.ref) {
			*s->l << lea(ptrC, fnParams);
			*s->l << lea(ptrA, v);
			*s->l << fnParam(ptrC);
			*s->l << fnParam(ptrConst(Offset()));
			*s->l << fnParam(ptrConst(Offset()));
			*s->l << fnParam(ptrConst(type.size()));
			*s->l << fnParam(byteConst(0));
			*s->l << fnParam(ptrA);
			*s->l << fnCall(e.ref(Engine::rFnParamsAdd), valVoid());
		} else if (type.isBuiltIn()) {
			*s->l << lea(ptrC, fnParams);
			*s->l << lea(ptrA, v);
			*s->l << fnParam(ptrC);
			*s->l << fnParam(ptrConst(Offset()));
			*s->l << fnParam(ptrConst(Offset()));
			*s->l << fnParam(ptrConst(type.size()));
			*s->l << fnParam(byteConst(type.isFloat() ? 1 : 0));
			*s->l << fnParam(ptrA);
			*s->l << fnCall(e.ref(Engine::rFnParamsAdd), valVoid());
		} else {
			// Value.
			code::Operand dtor = type.destructor();
			if (dtor.empty())
				dtor = ptrConst(Offset());
			*s->l << lea(ptrC, fnParams);
			*s->l << lea(ptrA, v);
			*s->l << fnParam(ptrC);
			*s->l << fnParam(type.copyCtor());
			*s->l << fnParam(dtor);
			*s->l << fnParam(ptrConst(type.size()));
			*s->l << fnParam(byteConst(0));
			*s->l << fnParam(ptrA);
			*s->l << fnCall(e.ref(Engine::rFnParamsAdd), valVoid());
		}
	}

	void addFnParamCopy(CodeGen *s, code::Var fnParams, Value type, code::Operand v) {
		using namespace code;
		Engine &e = s->engine();

		if (type.isBuiltIn() || type.ref) {
			if (type.ref) {
				TODO(L"Should we really allow pretending to deep-copy references?");
			}

			// Nothing special!
			addFnParam(s, fnParams, type, v);
		} else if (type.isActor()) {
			// Nothing special here!
			addFnParam(s, fnParams, type, v);
		} else if (type.isHeapObj()) {
			Var clone = s->createVar(type).v;
			*s->l << fnParam(v);
			*s->l << fnCall(cloneFn(type.type)->ref(), valPtr());
			*s->l << mov(clone, ptrA);

			// Regular parameter add.
			addFnParam(s, fnParams, type, clone);
		} else {
			VarInfo clone = s->createVar(type);
			*s->l << lea(ptrC, clone.v);
			*s->l << lea(ptrA, v);
			*s->l << fnParam(ptrC);
			*s->l << fnParamRef(ptrA, type.size(), type.copyCtor());
			*s->l << fnCall(cloneFn(type.type)->ref(), valVoid());
			clone.created(s);

			// Regular parameter add.
			addFnParam(s, fnParams, type, clone.v);
		}
	}

	static void allocNormalObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to) {
		using namespace code;

		Type *type = ctor->params->at(0).type;
		assert(Value(type).isHeapObj(), L"Must allocate a class type! (not " + ::toS(type) + L")");

		Engine &e = ctor->engine();

		*s->l << fnParam(type->typeRef());
		*s->l << fnCall(e.ref(Engine::rAlloc), valPtr());
		*s->l << mov(to, ptrA);

		CodeResult *r = new (s) CodeResult();
		params = new (s) Array<code::Operand>(*params);
		params->insert(0, to);
		ctor->autoCall(s, params, r);
	}

	static void allocRawObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to) {
		using namespace code;

		Type *type = ctor->params->at(0).type;
		Engine &e = ctor->engine();

		*s->l << lea(ptrA, to);

		CodeResult *r = new (s) CodeResult();
		params = new (s) Array<code::Operand>(*params);
		params->insert(0, ptrA);
		ctor->autoCall(s, params, r);
	}

	void allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params, code::Var to) {
		Value t = ctor->params->at(0);

		if (t.type->typeFlags & typeRawPtr)
			allocRawObject(s, ctor, params, to);
		else
			allocNormalObject(s, ctor, params, to);
	}

	code::Var allocObject(CodeGen *s, Function *ctor, Array<code::Operand> *params) {
		code::Var r = s->l->createVar(s->block, Size::sPtr);
		allocObject(s, ctor, params, r);
		return r;
	}

	code::Var allocObject(CodeGen *s, Type *type) {
		Function *ctor = type->defaultCtor();
		if (!ctor)
			throw InternalError(L"Can not allocate " + ::toS(type->identifier()) + L" using the default constructor.");
		return allocObject(s, ctor, new (type->engine) Array<code::Operand>());
	}

}
