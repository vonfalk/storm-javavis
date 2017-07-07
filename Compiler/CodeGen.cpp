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


	code::Var createFnCallParams(CodeGen *to, Nat paramCount) {
		using namespace code;

		Size s = Size::sPtr * paramCount;
		return to->l->createVar(to->block, s);
	}

	void setFnParam(CodeGen *to, code::Var params, Nat paramId, code::Operand param) {
		using namespace code;

		Offset o = Offset::sPtr * paramId;

		*to->l << lea(ptrA, param);
		*to->l << mov(ptrRel(params, o), ptrA);
	}

	static bool needsCloneEnv(Value type) {
		if (type.isBuiltIn() || type.ref)
			return false;
		else if (type.isActor())
			return false;
		else
			return true;
	}

	static code::Operand cloneParam(CodeGen *to, Value formal, code::Operand actual, code::Var env) {
		using namespace code;

		if (!needsCloneEnv(formal))
			return actual;

		if (formal.isHeapObj()) {
			VarInfo clone = to->createVar(formal);

			*to->l << fnParam(actual);
			*to->l << fnParam(env);
			*to->l << fnCall(cloneFnEnv(formal.type)->ref(), valPtr());
			*to->l << mov(clone.v, ptrA);
			clone.created(to);

			return clone.v;
		} else {
			Function *toCall = formal.type->deepCopyFn();
			if (!toCall)
				// No 'deepCopy'? No problem!
				return actual;

			VarInfo clone = to->createVar(formal);

			*to->l << lea(ptrC, clone.v);
			*to->l << lea(ptrA, actual);
			*to->l << fnParam(ptrC);
			*to->l << fnParam(ptrA);
			*to->l << fnCall(formal.copyCtor(), valVoid());
			clone.created(to);

			*to->l << lea(ptrC, clone.v);
			*to->l << fnParam(ptrC);
			*to->l << fnParam(env);
			*to->l << fnCall(toCall->ref(), valVoid());
			return clone.v;
		}
	}

	code::Var STORM_FN createFnCall(CodeGen *to, Array<Value> *formals, Array<code::Operand> *actuals, Bool copy) {
		using namespace code;
		assert(formals->count() == actuals->count(), L"Size of formals array does not match actuals!");

		// Examine if we need to create a CloneEnv.
		bool needEnv = false;
		for (Nat i = 0; i < formals->count(); i++)
			needEnv |= needsCloneEnv(formals->at(i));

		// Create it if required.
		Var env;
		if (needEnv)
			env = allocObject(to, CloneEnv::stormType(to->engine()));

		// Create the FnCall array.
		Var call = createFnCallParams(to, formals->count());

		// Start copying the parameters...
		for (Nat i = 0; i < formals->count(); i++) {
			Operand clone = actuals->at(i);
			if (copy)
				clone = cloneParam(to, formals->at(i), clone, env);
			setFnParam(to, call, i, clone);
		}

		return call;
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

	static void addThunkParam(code::Listing *l, Value param, Nat id) {
		using namespace code;

		Offset o = Offset::sPtr * id;
		if (param.isValue() && !param.isBuiltIn()) {
			*l << fnParamRef(ptrRel(ptrB, o), param.size(), param.copyCtor());
		} else {
			*l << fnParamRef(ptrRel(ptrB, o), param.size());
		}
	}

	static void addThunkResult(code::Listing *l, Value result, code::Var res) {
		using namespace code;

		if (!result.returnInReg())
			*l << fnParam(res);
	}

	static void addThunkCall(CodeGen *s, Value result, code::Var res, code::Var fn) {
		using namespace code;

		*s->l << fnCall(fn, result.valTypeRet());
		if (result.returnInReg() && result != Value()) {
			*s->l << mov(ptrB, res);
			*s->l << mov(xRel(result.size(), ptrB, Offset()), asSize(ptrA, result.size()));
		}

	}

	code::Binary *callThunk(Value result, Array<Value> *formals) {
		using namespace code;
		Engine &e = formals->engine();

		CodeGen *s = new (e) CodeGen(RunOn());
		Listing *l = s->l;

		// Parameters:
		Var fn = l->createParam(valPtr());
		Var member = l->createParam(ValType(Size::sByte, false));
		Var params = l->createParam(valPtr());
		Var first = l->createParam(valPtr());
		Var output = l->createParam(valPtr());

		*l << prolog();
		*l << mov(ptrB, params);
		// Do the function call. We have four alternatives:
		// 1: Neither a first param, nor member (lPlain).
		// 2: We have a first param, but not a member (lFirst).
		// 3: No first param, but a member (lMember).
		// 4: Both a first param and a member (lBoth).
		Label lNoMember = l->label();
		Label lPlain = l->label();
		Label lFirst = l->label();
		Label lMember = l->label();
		Label lBoth = l->label();

		if (!result.returnInReg()) {
			// These cases (lBoth and lMember) can be ignored if we are returning something in a register.
			*l << cmp(member, byteConst(0));
			*l << jmp(lNoMember, ifEqual);
			*l << cmp(first, ptrConst(0));
			*l << jmp(lMember, ifEqual);

			*l << lBoth;
			*l << fnParam(first);
			addThunkResult(l, result, output);
			for (Nat i = 0; i < formals->count(); i++)
				addThunkParam(l, formals->at(i), i);
			addThunkCall(s, result, output, fn);
			*l << epilog();
			*l << ret(valVoid());

			*l << lMember;
			if (formals->count() > 0)
				// Not really legal, but anyway...
				addThunkParam(l, formals->at(0), 0);
			addThunkResult(l, result, output);
			for (Nat i = 1; i < formals->count(); i++)
				addThunkParam(l, formals->at(i), i);
			addThunkCall(s, result, output, fn);
			*l << epilog();
			*l << ret(valVoid());
		}

		*l << lNoMember;
		*l << cmp(first, ptrConst(0));
		*l << jmp(lPlain, ifEqual);

		*l << lFirst;
		addThunkResult(l, result, output);
		*l << fnParam(first);
		for (Nat i = 0; i < formals->count(); i++)
			addThunkParam(l, formals->at(i), i);
		addThunkCall(s, result, output, fn);
		*l << epilog();
		*l << ret(valVoid());

		*l << lPlain;
		addThunkResult(l, result, output);
		for (Nat i = 0; i < formals->count(); i++)
			addThunkParam(l, formals->at(i), i);
		addThunkCall(s, result, output, fn);
		*l << epilog();
		*l << ret(valVoid());

		return new (e) Binary(e.arena(), l);
	}

}
