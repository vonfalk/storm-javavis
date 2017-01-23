#include "stdafx.h"
#include "CodeGen.h"
#include "Exception.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"

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
		frame(o->frame),
		retType(o->retType),
		returnParam(o->returnParam) {}

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

	wrap::Variable CodeGen::addParam(Value type) {
		return addParam(type, false);
	}

	wrap::Variable CodeGen::addParam(Value type, Bool addRef) {
		using namespace code;

		if (type.isValue()) {
			return frame.createParameter(type.size(), type.isFloat(), type.destructor(), freeOnBoth | freePtr);
		} else if (addRef) {
			Variable v = frame.createParameter(type.size(), false, type.destructor());
			if (type.refcounted())
				to << code::addRef(v);
			return v;
		} else {
			return frame.createParameter(type.size(), false);
		}
	}

	void CodeGen::returnType(Value type, Bool isMember) {
		assert(retType == Value(), L"Trying to re-set the return type of CodeGen.");
		retType = type;

		if (type.isValue()) {
			// We need a return value parameter as either the first or the second parameter!
			returnParam = frame.createParameter(Size::sPtr, false);
			frame.moveParam(returnParam, isMember ? 1 : 0);
		}
	}

	Value CodeGen::returnType() {
		return retType;
	}

	void CodeGen::returnValue(wrap::Variable value) {
		using namespace code;

		if (retType == Value()) {
			to << epilog();
			to << ret(retVoid());
		} else if (retType.returnInReg()) {
			to << mov(asSize(ptrA, retType.size()), value.v);
			if (retType.refcounted())
				to << code::addRef(ptrA);
			to << epilog();
			to << ret(retType.retVal());
		} else {
			to << lea(ptrA, ptrRel(value.v));
			to << fnParam(returnParam);
			to << fnParam(ptrA);
			to << fnCall(retType.copyCtor(), retPtr());

			// We need to provide the address of the return value as our result. The copy ctor does
			// not neccessarily return an address to the created value. This is important in some
			// optimized builds, where the compiler assumes that ptrA contains the address of the
			// returned value. This is usually not the case in unoptimized builds.
			to << mov(ptrA, returnParam);
			to << epilog();
			to << ret(retPtr());
		}
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
			// Generate a temporary variable. Do not save it, it will mess up cases like
			// if, where two different branches may end up in different blocks!
			return storm::variable(s, t);
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

	wrap::Variable allocObject(Par<CodeGen> s, Par<Function> ctor, Par<Array<wrap::Operand>> params) {
		vector<code::Value> p(params->count());
		for (nat i = 0; i < params->count(); i++)
			p[i] = params->at(i).v;

		return allocObject(s, ctor, p);
	}

	code::Variable allocObject(Par<CodeGen> s, Par<Function> ctor, const vector<code::Value> &params) {
		Engine &e = s->engine();
		code::Variable r = s->frame.createPtrVar(s->block.v, e.fnRefs.release);
		allocObject(s, ctor, params, r);
		return r;
	}

	static void allocNormalObject(Par<CodeGen> s, Par<Function> ctor, vector<code::Value> params, code::Variable to) {
		using namespace code;

		Type *type = ctor->params[0].type;
		Engine &e = ctor->engine();

		Block b = s->frame.createChild(s->frame.last(s->block.v));
		Variable rawMem = s->frame.createPtrVar(b, e.fnRefs.freeRef, freeOnException);

		s->to << begin(b);
		s->to << fnParam(type->typeRef);
		s->to << fnCall(e.fnRefs.allocRef, retPtr());
		s->to << mov(rawMem, ptrA);

		Auto<CodeResult> r = CREATE(CodeResult, s);
		params.insert(params.begin(), code::Value(rawMem));
		ctor->autoCall(steal(s->child(b)), params, r);

		s->to << mov(to, rawMem);
		s->to << end(b);
	}

	static void allocRawObject(Par<CodeGen> s, Par<Function> ctor, vector<code::Value> params, code::Variable to) {
		using namespace code;

		Type *type = ctor->params[0].type;
		Engine &e = ctor->engine();

		s->to << lea(ptrA, to);

		Auto<CodeResult> r = CREATE(CodeResult, s);
		params.insert(params.begin(), code::Value(ptrA));
		ctor->autoCall(s, params, r);
	}

	void allocObject(Par<CodeGen> s, Par<Function> ctor, Par<Array<wrap::Operand>> params, wrap::Variable to) {
		vector<code::Value> p(params->count());
		for (nat i = 0; i < params->count(); i++)
			p[i] = params->at(i).v;

		allocObject(s, ctor, p, to.v);
	}

	void allocObject(Par<CodeGen> s, Par<Function> ctor, const vector<code::Value> &params, code::Variable to) {
		Type *type = ctor->params[0].type;
		assert(type->typeFlags & typeClass, L"Must allocate class types.");

		if (type->typeFlags & typeRawPtr)
			allocRawObject(s, ctor, params, to);
		else
			allocNormalObject(s, ctor, params, to);
	}

	wrap::Variable createFnParams(Par<CodeGen> s, wrap::Operand memory) {
		using namespace code;

		Engine &e = s->engine();
		Variable v = s->frame.createVariable(s->block.v,
											fnParamsSize(),
											e.fnRefs.fnParamsDtor,
											freeOnBoth | freePtr);
		// Call the ctor!
		s->to << lea(ptrC, v);
		s->to << fnParam(ptrC);
		s->to << fnParam(memory.v);
		s->to << fnCall(e.fnRefs.fnParamsCtor, retVoid());

		return v;
	}

	wrap::Variable createFnParams(Par<CodeGen> s, Nat params) {
		using namespace code;
		// Allocate space for the parameters on the stack.
		Size total = fnParamSize() * params;
		Variable v = s->frame.createVariable(s->block.v, total);
		s->to << lea(ptrA, v);

		return createFnParams(s, code::Value(ptrA));
	}

	// Find 'std:clone' for the given type.
	wrap::Operand stdCloneFn(const Value &type) {
		if (type == Value())
			return wrap::Operand();

		Engine &e = type.type->engine;
		Auto<Name> name = CREATE(Name, e);
		name->add(L"core");
		name->add(L"clone", valList(1, type));
		Auto<Function> f = steal(e.scope()->find(name)).as<Function>();
		if (!f)
			throw InternalError(L"Could not find std.clone(" + ::toS(type) + L").");
		return code::Value(f->ref());
	}

	void STORM_FN addFnParam(Par<CodeGen> s, wrap::Variable fnParams, const Value &type,
							const wrap::Operand &v, Bool thunk) {
		using namespace code;
		Engine &e = s->engine();

		if (type.isClass() && thunk) {
			s->to << lea(ptrC, fnParams.v);
			s->to << lea(ptrA, v.v);
			s->to << fnParam(ptrC);
			s->to << fnParam(e.fnRefs.copyRefPtr);
			s->to << fnParam(e.fnRefs.releasePtr);
			s->to << fnParam(natConst(type.size()));
			s->to << fnParam(byteConst(0));
			s->to << fnParam(ptrA);
			s->to << fnCall(e.fnRefs.fnParamsAdd, retVoid());
		} else if (type.isClass() && !thunk) {
			s->to << lea(ptrC, fnParams.v);
			s->to << fnParam(ptrC);
			s->to << fnParam(intPtrConst(0));
			s->to << fnParam(intPtrConst(0));
			s->to << fnParam(natConst(type.size()));
			s->to << fnParam(byteConst(0));
			s->to << fnParam(v.v);
			s->to << fnCall(e.fnRefs.fnParamsAdd, retVoid());
		} else if (type.ref || type.isBuiltIn()) {
			s->to << lea(ptrC, fnParams.v);
			s->to << fnParam(ptrC);
			s->to << fnParam(intPtrConst(0));
			s->to << fnParam(intPtrConst(0));
			s->to << fnParam(natConst(type.size()));
			s->to << fnParam(byteConst(type.isFloat() ? 1 : 0));
			s->to << fnParam(v.v);
			s->to << fnCall(e.fnRefs.fnParamsAdd, retVoid());
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
			s->to << fnParam(byteConst(0));
			s->to << fnParam(ptrA);
			s->to << fnCall(e.fnRefs.fnParamsAdd, retVoid());
		}
	}

	void STORM_FN addFnParamCopy(Par<CodeGen> s, wrap::Variable fnParams, const Value &type,
								const wrap::Operand &v, Bool thunk) {
		using namespace code;
		Engine &e = s->engine();

		if (type.isClass()) {
			Variable clone = s->frame.createPtrVar(s->block.v, e.fnRefs.release);
			s->to << fnParam(v.v);
			s->to << fnCall(stdCloneFn(type).v, retPtr());
			s->to << mov(clone, ptrA);

			// Regular parameter add.
			addFnParam(s, fnParams, type, wrap::Variable(clone), thunk);
		} else if (type.ref || type.isBuiltIn()) {
			if (type.ref) {
				TODO(L"Should we really allow pretending to deep-clone references?");
			}

			// Reuse the plain one.
			addFnParam(s, fnParams, type, v, thunk);
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
			s->to << fnParam(byteConst(0));
			s->to << fnParam(ptrA);
			s->to << fnCall(e.fnRefs.fnParamsAdd, retVoid());
		}
	}

	wrap::Variable STORM_FN valueToS(Par<CodeGen> s, wrap::Variable object, Value type) {
		using namespace code;
		Engine &e = s->engine();

		Value str = value<Str *>(e);
		Value thisPtr = Value::thisPtr(type.type);
		Auto<Function> fn = steal(type.type->findCpp(L"toS", valList(1, thisPtr))).as<Function>();
		if (!fn)
			throw InternalError(L"The type " + ::toS(thisPtr) + L" does not have a toS function.");

		code::Value par;
		if (!type.ref && type.isValue()) {
			s->to << lea(ptrA, object.v);
			par = ptrA;
		} else {
			par = object.v;
		}

		Auto<CodeResult> result = CREATE(CodeResult, s, str, s->block);
		fn->autoCall(s, vector<code::Value>(1, par), result);
		return result->location(s).v;
	}

}