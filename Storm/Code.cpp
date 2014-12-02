#include "stdafx.h"
#include "Code.h"
#include "Function.h"
#include "Exception.h"
#include "Code/Redirect.h"

namespace storm {

	Code::Code() : owner(null) {}

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
		toUpdate->set(ptr);
	}


	/**
	 * Lazily loaded code.
	 */

	LazyCode::LazyCode(const Fn<code::Listing, void> &fn) : code(null), loaded(false), loading(false), load(fn) {}

	LazyCode::~LazyCode() {
		if (code)
			engine().destroy(code);
	}

	void LazyCode::newRef() {
		if (!code)
			createRedirect();
		code->update(*toUpdate);
	}

	void LazyCode::createRedirect() {
		code::Redirect r;
		loaded = false;

		r.result(owner->result.size(), owner->result.isBuiltIn());

		// parameters
		for (nat i = 0; i < owner->params.size(); i++) {
			r.param(owner->params[i].size(), owner->params[i].destructor());
		}

		engine().lazyCodeFn.set(address(&LazyCode::updateCode));
		setCode(r.code(code::Ref(engine().lazyCodeFn), code::ptrConst(this)));
	}

	const void *LazyCode::updateCode(LazyCode *c) {
		if (!c->loaded) {
			if (c->loading)
				throw InternalError(L"Trying to update " + c->owner->identifier() + L" recursively!");

			c->loading = true;
			c->setCode(c->load());
			c->loading = false;
			c->loaded = true;
		}

		return c->code->getData();
	}

	void LazyCode::setCode(const code::Listing &l) {
		if (code)
			engine().destroy(code);
		code = new code::Binary(engine().arena, L"redirect", l);
		if (toUpdate)
			code->update(*toUpdate);
	}


	/**
	 * Delegated code.
	 */

	DelegatedCode::DelegatedCode(code::Ref ref, const String &title) : reference(ref, title) {
		reference.onChange = memberFn(this, &DelegatedCode::updated);
	}

	void DelegatedCode::newRef() {
		updated(reference.address());
	}

	void DelegatedCode::updated(void *p) {
		if (toUpdate)
			toUpdate->set(p);
	}


	/**
	 * Inlined code.
	 */

	InlinedCode::InlinedCode(Fn<void, InlinedParams> gen)
		: LazyCode(memberVoidFn(this, &InlinedCode::generatePtr)), generate(gen) {}

	void InlinedCode::code(const GenState &state, const vector<code::Value> &params, code::Value result) {
		InlinedParams p = { state, params, result };
		generate(p);
	}

	code::Listing InlinedCode::generatePtr() {
		using namespace code;
		if (!owner->result.isBuiltIn()) {
			TODO(L"Implement return of non-built in types");
			assert(false);
		}

		Listing l;

		vector<code::Value> params;
		for (nat i = 0; i < owner->params.size(); i++) {
			const Value &p = owner->params[i];
			params.push_back(l.frame.createParameter(p.size(), false, p.destructor()));
		}

		l << prolog();

		GenState state = { l, l.frame, l.frame.root() };
		code(state, params, asSize(ptrA, owner->result.size()));

		l << epilog();
		l << ret(owner->result.size());

		return l;
	}
}
