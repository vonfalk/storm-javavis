#include "stdafx.h"
#include "Code.h"
#include "Function.h"
#include "Exception.h"
#include "Engine.h"
#include "Code/Redirect.h"
#include "Code/Future.h"

namespace storm {

	Code::Code() : owner(null), toUpdate(null) {}

	void Code::detach() {
		owner = null;
	}

	void Code::attach(Function *fn) {
		owner = fn;
	}

	void Code::update(code::RefSource &update) {
		if (toUpdate == &update)
			return;
		toUpdate = &update;
		newRef();
	}


	/**
	 * Static code.
	 */

	StaticCode::StaticCode(void *ptr) : ptr(ptr) {}

	void StaticCode::newRef() {
		toUpdate->setPtr(ptr);
	}


	/**
	 * Dynamic code.
	 */

	DynamicCode::DynamicCode(const code::Listing &src) {
		code = new code::Binary(engine().arena, src);
	}

	DynamicCode::~DynamicCode() {
		if (code)
			engine().destroy(code);
	}

	void DynamicCode::newRef() {
		toUpdate->set(code);
	}


	/**
	 * Lazily loaded code.
	 */

	LazyCode::LazyCode(Par<FnPtr<CodeGen *>> fn) : code(null), loaded(false), loading(false), load(fn) {}

	LazyCode::~LazyCode() {
		if (code)
			engine().destroy(code);
	}

	void LazyCode::newRef() {
		if (!code)
			createRedirect();
		toUpdate->set(code);
	}

	void LazyCode::createRedirect() {
		code::Redirect r;
		loaded = false;

		r.result(owner->result.size(), owner->result.returnInReg());

		// parameters (no refcount on parameters), but we need to destroy value parameters
		// if there is an exception...
		for (nat i = 0; i < owner->params.size(); i++) {
			if (owner->params[i].isValue()) {
				r.param(owner->params[i].size(), owner->params[i].destructor(), true);
			} else {
				r.param(owner->params[i].size(), code::Value(), false);
			}
		}

		code::RefSource &src = engine().fnRefs.lazyCodeFn;
		src.setPtr(address(&LazyCode::updateCode));
		setCode(r.code(engine().fnRefs.lazyCodeFn, owner->isMember(), code::ptrConst(this)));
	}

	const void *LazyCode::updateCode(LazyCode *c) {

		// If we're on the compiler thread, we may call directly.
		// TODO? Always allocate a new UThread? This will make sure the compiler
		// always has a predictable amount of stack space in some causes, which could be beneficial.
		Thread *cThread = Compiler::thread(c->engine());
		if (cThread->thread != code::Thread::current()) {
			// Note, we're blocking the calling thread entirely since we would otherwise
			// possibly let other UThreads run where it was not expected!
			code::Future<const void *, Semaphore> result;
			code::UThread::spawn(&LazyCode::updateCodeLocal, true, code::FnParams().add(c), result, &cThread->thread);
			return result.result();
		} else {
			return c->updateCodeLocal(c);
		}
	}

	const void *LazyCode::updateCodeLocal(LazyCode *c) {
		if (!c->loaded) {
			if (c->loading)
				throw InternalError(L"Trying to update " + c->owner->identifier() + L" recursively!");

			c->loading = true;
			try {
				Auto<CodeGen> r = c->load->call();
				r->to << *r->data;
				c->setCode(r->to);
			} catch (...) {
				c->loading = false;
				throw;
			}
			c->loading = false;
			c->loaded = true;
		}

		return c->code->address();
	}

	void LazyCode::setCode(const code::Listing &l) {
		// PLN("New code for " << owner->identifier() << ":" << l);
		code::Binary *newCode = new code::Binary(engine().arena, l);

		if (code)
			engine().destroy(code);
		code = newCode;
		if (toUpdate)
			toUpdate->set(code);
	}


	/**
	 * Delegated code.
	 */

	DelegatedCode::DelegatedCode(code::Ref ref) :
		content(new code::Content(engine().arena)),
		reference(ref, *content),
		added(false) {
		reference.onChange = memberFn(this, &DelegatedCode::updated);
		content->set(ref.address());
	}

	DelegatedCode::~DelegatedCode() {
		if (!added)
			delete content;
	}

	void DelegatedCode::newRef() {
		toUpdate->set(content);
		added = true;
	}

	void DelegatedCode::updated(void *p) {
		content->set(p);
	}


	/**
	 * Inlined code.
	 */

	InlinedCode::InlinedCode(Fn<void, InlinedParams> gen)
		: LazyCode(steal(memberWeakPtr(engine(), this, &InlinedCode::generatePtr))), generate(gen) {}

	void InlinedCode::code(Par<CodeGen> state, const vector<code::Value> &params, Par<CodeResult> result) {
		InlinedParams p = { engine(), state, params, result };
		generate(p);
	}

	CodeGen *InlinedCode::generatePtr() {
		using namespace code;
		if (!owner->result.returnInReg()) {
			TODO(L"Implement return of non-built in types");
			assert(false);
		}

		Auto<CodeGen> state = CREATE(CodeGen, this, owner->runOn());
		Listing &l = state->to;

		vector<code::Value> params;
		for (nat i = 0; i < owner->params.size(); i++) {
			const Value &p = owner->params[i];
			params.push_back(l.frame.createParameter(p.size(), false, p.destructor()));
		}

		l << prolog();

		Auto<CodeResult> result = CREATE(CodeResult, this, owner->result, l.frame.root());
		code(state, params, result);

		if (owner->result != Value()) {
			l << mov(asSize(ptrA, owner->result.size()), result->location(state).var());
		}

		l << epilog();
		l << ret(owner->result.size());

		return state.ret();
	}


	/**
	 * StaticEngineCode.
	 */

	StaticEngineCode::StaticEngineCode(const Value &returnType, void *ptr) : original(engine().arena, L"ref-to") {
		using namespace code;

		original.setPtr(ptr);

		Listing l = enginePtrThunk(engine(), returnType, original);
		code = new Binary(engine().arena, l);
	}

	StaticEngineCode::~StaticEngineCode() {
		engine().destroy(code);
	}

	void StaticEngineCode::newRef() {
		toUpdate->set(code);
	}

}
