#include "stdafx.h"
#include "Code.h"
#include "Engine.h"
#include "Function.h"
#include "Exception.h"
#include "Engine.h"
#include "Core/Str.h"

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

	StaticEngineCode::StaticEngineCode(const void *src) {
		// TODO: Better reference somehow?
		original = new (this) code::StrRefSource(S("ref-to"));
		original->setPtr(src);

		code = null;
	}

	void StaticEngineCode::newRef() {
		if (!code) {
			code::Listing *l = redirectCode(original);
			code = new (this) code::Binary(engine().arena(), l);
		}
		toUpdate->set(code);
	}

	code::Listing *StaticEngineCode::redirectCode(code::Ref ref) {
		Engine &e = engine();
		assert(owner);
		using code::TypeDesc;

		Array<TypeDesc *> *p = new (this) Array<TypeDesc *>();
		Array<Value> *params = owner->params;
		for (Nat i = 0; i < params->count(); i++)
			p->push(params->at(i).desc(e));
		Value result = owner->result;

		return e.arena()->engineRedirect(result.desc(e), p, ref, e.ref(Engine::rEngine));
	}


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
		Engine &e = engine();
		using code::TypeDesc;

		Array<TypeDesc *> *params = new (this) Array<TypeDesc *>();
		for (Nat i = 0; i < owner->params->count(); i++) {
			params->push(owner->params->at(i).desc(e));
		}

		Bool member = owner->isMember();
		TypeDesc *result = owner->result.desc(e);
		code::Ref fn = e.ref(Engine::rLazyCodeUpdate);
		setCode(e.arena()->redirect(member, result, params, fn, code::objPtr(this)));
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
			os::FnCall<const void *, 1> params = os::fnCall().add(me);
			os::UThread::spawn(address(&LazyCode::updateCodeLocal), true, params, result, &cThread->thread());
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
				me->setCode(l->l);
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

	InlineCode::InlineCode(Fn<void, InlineParams> *create) :
		LazyCode(fnPtr(TObject::engine(), &InlineCode::generatePtr, this)), create(create) {}

	void InlineCode::code(CodeGen *state, Array<code::Operand> *params, CodeResult *result) {
		InlineParams p(state, params, result);
		create->call(p);
	}

	CodeGen *InlineCode::generatePtr() {
		using namespace code;

		CodeGen *state = new (this) CodeGen(owner->runOn(), owner->isMember(), owner->result);
		Listing *l = state->l;

		Array<code::Operand> *params = new (this) Array<code::Operand>();
		for (nat i = 0; i < owner->params->count(); i++) {
			Value p = owner->params->at(i);
			params->push(state->createParam(p));
		}

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
