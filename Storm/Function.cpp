#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	Function::Function(Value result, const String &name, const vector<Value> &params)
		: NameOverload(name, params), result(result), lookupRef(null), codeRef(null) {}

	Function::~Function() {
		// Correct destruction order.
		code = null;
		lookup = null;
		delete codeRef;
		delete lookupRef;
	}

	void *Function::pointer() {
		return ref().address();
	}

	code::RefSource &Function::ref() {
		initRefs();
		return *lookupRef;
	}

	void Function::setCode(Auto<Code> code) {
		if (this->code)
			this->code->detach();
		code->attach(this);
		this->code = code;
		if (codeRef)
			code->update(*codeRef);
	}

	void Function::setLookup(Auto<Code> code) {
		if (lookup)
			lookup->detach();
		code->attach(this);
		lookup = code;
		if (lookupRef)
			lookup->update(*lookupRef);
	}

	void Function::initRefs() {
		if (!codeRef) {
			assert(("Too early!", parent()));
			codeRef = new code::RefSource(engine().arena, identifier() + L"<c>");
			if (code)
				code->update(*codeRef);
		}

		if (!lookupRef) {
			assert(("Too early!", parent()));
			lookupRef = new code::RefSource(engine().arena, identifier() + L"<l>");
			if (!lookup)
				lookup = CREATE(DelegatedCode, engine(), code::Ref(*codeRef), lookupRef->getTitle());
			lookup->update(*lookupRef);
		}
	}

	void Function::output(wostream &to) const {
		to << result << " " << name << "(";
		join(to, params, L", ");
		to << ")";
	}

	Function *nativeFunction(Engine &e, Value result, const String &name, const vector<Value> &params, void *ptr) {
		Function *fn = CREATE(Function, e, result, name, params);
		fn->setCode(CREATE(StaticCode, e, ptr));
		return fn;
	}

}
