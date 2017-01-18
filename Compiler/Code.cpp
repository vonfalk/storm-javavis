#include "stdafx.h"
#include "Code.h"
#include "Engine.h"
#include "Function.h"
#include "Exception.h"
#include "Core/Str.h"
#include "Code/Redirect.h"

namespace storm {

	Code::Code() : toUpdate(null), owner(null) {}

	void Code::attach(Function *to) {
		owner = to;
	}

	void Code::detach() {
		owner = null;
	}

	void Code::update(code::RefSource *update) {
		if (toUpdate == update)
			return;
		toUpdate = update;
		newRef();
	}

	void Code::compile() {}

	void Code::newRef() {}


	/**
	 * Static code.
	 */

	StaticCode::StaticCode(const void *ptr) : ptr(ptr) {}

	void StaticCode::newRef() {
		toUpdate->setPtr(ptr);
	}


	/**
	 * Dynamic code.
	 */

	DynamicCode::DynamicCode(code::Listing *code) {
		binary = new (this) code::Binary(engine().arena(), code);
	}

	void DynamicCode::newRef() {
		toUpdate->set(binary);
	}


	/**
	 * Delegated code.
	 */

	DelegatedCode::DelegatedCode(code::Ref ref) {
		content = new (this) code::DelegatedContent(ref);
	}

	void DelegatedCode::newRef() {
		toUpdate->set(content);
	}


	/**
	 * Static engine code.
	 */

	StaticEngineCode::StaticEngineCode(Value result, const void *src) {
		original = new (this) code::RefSource(L"ref-to");
		original->setPtr(src);

		code::Listing *l = redirectCode(result, original);
		code = new (this) code::Binary(engine().arena(), l);
	}

	void StaticEngineCode::newRef() {
		toUpdate->set(code);
	}

#ifdef X86

	code::Listing *StaticEngineCode::redirectCode(Value result, code::Ref ref) {
		using namespace code;

		Listing *l = new (this) Listing();

		if (result.returnInReg()) {
			// The old pointer and the 0 constant will nicely fit into the 'returnData' member.
			*l << push(ptrConst(Offset(0)));
			*l << push(engine().ref(Engine::rEngine));
		} else {
			// The first parameter is, and has to be, a pointer to the returned object.
			*l << mov(ptrA, ptrRel(ptrStack, Offset::sPtr)); // Read the return value ptr.
			*l << push(engine().ref(Engine::rEngine));
			*l << push(ptrA); // Store the return value ptr once more.
		}

		*l << call(ref, result.valTypeRet());
		*l << add(ptrStack, ptrConst(Size::sPtr * 2));
		*l << ret(result.valTypeRet());

		return l;
	}

#else
#error "Please implement 'redirectCode' for your platform!"
#endif


	/**
	 * Lazy code.
	 */

	LazyCode::LazyCode(Fn<CodeGen *> *generate) : binary(null), generate(generate) {}

	void LazyCode::compile() {
		// We're always running on the Compiler thread, so it is safe to call 'updateCodeLocal'.
		if (!loading)
			updateCodeLocal(this);
	}

	void LazyCode::newRef() {
		if (!binary)
			createRedirect();
		toUpdate->set(binary);
	}

	void LazyCode::createRedirect() {
		loaded = false;

		Array<code::RedirectParam> *params = new (this) Array<code::RedirectParam>();
		for (nat i = 0; i < owner->params->count(); i++) {
			Value v = owner->params->at(i);
			if (v.isValue()) {
				params->push(code::RedirectParam(v.valTypeParam(), v.destructor(), true));
			} else {
				params->push(code::RedirectParam(v.valTypeParam()));
			}
		}

		setCode(code::redirect(params, engine().ref(Engine::rLazyCodeUpdate), code::objPtr(this)));
	}

	void LazyCode::setCode(code::Listing *to) {
		binary = new (this) code::Binary(engine().arena(), to);
		if (toUpdate)
			toUpdate->set(binary);
	}

	const void *LazyCode::updateCode(LazyCode *me) {
		// TODO? Always allocate a new UThread? This will make sure we don't run out of stack for the compiler.
		Thread *cThread = Compiler::thread(me->engine());
		if (cThread->thread() == os::Thread::current()) {
			// If we're on the Compiler thread, we may call directly.
			return updateCodeLocal(me);
		} else {
			// Note: we're blocking the calling thread entirely since we would otherwise possibly
			// let other UThreads run where they are not expected to.
			os::Future<const void *, Semaphore> result;
			os::UThread::spawn(&LazyCode::updateCodeLocal, true, os::FnParams().add(me), result, &cThread->thread());
			return result.result();
		}
	}

	const void *LazyCode::updateCodeLocal(LazyCode *me) {
		while (me->loading) {
			// Wait for the other one loading this function.
			// TODO: Try to detect when a function is recursively loaded!
			os::UThread::leave();
		}

		if (!me->loaded) {
			if (me->loading)
				throw InternalError(L"Trying to update " + ::toS(me->owner->identifier()) + L" recursively.");

			me->loading = true;

			try {
				CodeGen *l = me->generate->call();
				// Append any data needed.
				me->setCode(l->to);
			} catch (...) {
				me->loading = false;
				throw;
			}

			me->loaded = true;
			me->loading = false;
		}

		return me->binary->address();
	}


	/**
	 * Inline code.
	 */

	InlineParams::InlineParams(CodeGen *state, Array<code::Operand> *params, CodeResult *result) :
		state(state), params(params), result(result) {}

	void InlineParams::deepCopy(CloneEnv *env) {
		clone(state, env);
		clone(params, env);
		clone(result, env);
	}


	InlineCode::InlineCode(Fn<void, InlineParams> *create) :
		LazyCode(fnPtr(TObject::engine(), &InlineCode::generatePtr, this)), create(create) {}

	void InlineCode::code(CodeGen *state, Array<code::Operand> *params, CodeResult *result) {
		InlineParams p(state, params, result);
		create->call(p);
	}

	CodeGen *InlineCode::generatePtr() {
		using namespace code;

		CodeGen *state = new (this) CodeGen(owner->runOn());
		Listing *l = state->to;

		Array<code::Operand> *params = new (this) Array<code::Operand>();
		for (nat i = 0; i < owner->params->count(); i++) {
			Value p = owner->params->at(i);
			params->push(state->createParam(p));
		}

		state->result(owner->result, owner->isMember());

		*l << prolog();

		if (owner->result == Value()) {
			CodeResult *result = new (this) CodeResult();
			code(state, params, result);
			state->returnValue(code::Var());
		} else {
			CodeResult *result = new (this) CodeResult(owner->result, l->root());
			code(state, params, result);

			VarInfo v = result->location(state);
			v.created(state);
			state->returnValue(v.v);
		}

		return state;
	}

}
