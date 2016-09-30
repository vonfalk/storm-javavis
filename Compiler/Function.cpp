#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Core/Str.h"

namespace storm {

	Function::Function(Value result, Str *name, Array<Value> *params) :
		Named(name, params),
		result(result),
		lookupRef(null),
		codeRef(null),
		runOnThread(null) {}

	const void *Function::pointer() {
		return ref()->address();
	}

	code::RefSource *Function::ref() {
		initRefs();
		return lookupRef;
	}

	code::RefSource *Function::directRef() {
		initRefs();
		return codeRef;
	}

	Bool Function::isMember() {
		return as<Type>(parent()) != null;
	}

	RunOn Function::runOn() const {
		if (Type *t = as<Type>(parent())) {
			return t->runOn();
		} else if (runOnThread) {
			return RunOn(runOnThread);
		} else {
			return RunOn(RunOn::any);
		}
	}

	void Function::runOn(NamedThread *t) {
		runOnThread = t;
	}

	void Function::toS(StrBuf *to) const {
		*to << result << L" ";
		Named::toS(to);
	}

	void Function::setCode(Code *code) {
		if (this->code)
			this->code->detach();
		code->attach(this);
		this->code = code;
		if (codeRef)
			this->code->update(codeRef);
	}

	void Function::setLookup(MAYBE(Code *) code) {
		if (lookup)
			lookup->detach();

		if (code == null && codeRef != null)
			lookup = new (this) DelegatedCode(code::Ref(codeRef));
		else
			lookup = code;

		if (lookup) {
			lookup->attach(this);
			if (lookupRef)
				lookup->update(lookupRef);
		}
	}

	void Function::compile() {
		if (code)
			code->compile();
		if (lookup)
			lookup->compile();
	}

	void Function::initRefs() {
		if (!codeRef) {
			assert(parent(), L"Too early!");
			codeRef = new (this) code::RefSource(*identifier() + L"<c>");
			if (code)
				code->update(codeRef);
		}

		if (!lookupRef) {
			assert(parent(), L"Too early!");
			lookupRef = new (this) code::RefSource(*identifier() + L"<l>");
			if (!lookup) {
				lookup = new (this) DelegatedCode(code::Ref(codeRef));
				lookup->attach(this);
			}
			lookup->update(lookupRef);
		}
	}

}
