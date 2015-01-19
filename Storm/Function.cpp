#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"
#include "Exception.h"
#include "Code/Instruction.h"

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

	void Function::genCode(const GenState &to, const vector<code::Value> &params, GenResult &res) {
		using namespace code;

		initRefs();

		assert(params.size() == this->params.size());
		bool inlined = true;
		inlined &= as<DelegatedCode>(lookup.borrow()) != 0;
		inlined &= as<InlinedCode>(code.borrow()) != 0;

		if (inlined) {
			InlinedCode *c = as<InlinedCode>(code.borrow());
			c->code(to, params, res);
		} else if (result == Value()) {
			assert(("Not implemented for value types yet!", this->result.isBuiltIn()));
			for (nat i = 0; i < params.size(); i++)
				to.to << fnParam(params[i]);

			to.to << fnCall(Ref(ref()), Size());
		} else {
			Variable result = res.safeLocation(to, this->result);

			assert(("Not implemented for value types yet!", this->result.isBuiltIn()));
			for (nat i = 0; i < params.size(); i++)
				to.to << fnParam(params[i]);

			to.to << fnCall(Ref(ref()), result.size());
			to.to << mov(result, asSize(ptrA, result.size()));
		}
	}

	void Function::setCode(Par<Code> code) {
		if (this->code)
			this->code->detach();
		code->attach(this);
		this->code = code;
		if (codeRef)
			code->update(*codeRef);
	}

	void Function::setLookup(Par<Code> code) {
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
		Auto<StaticCode> c = CREATE(StaticCode, e, ptr);
		fn->setCode(c);
		return fn;
	}

	Function *inlinedFunction(Engine &e, Value result, const String &name,
							const vector<Value> &params, Fn<void, InlinedParams> fn) {
		Function *f = CREATE(Function, e, result, name, params);
		Auto<InlinedCode> ic = CREATE(InlinedCode, e, fn);
		f->setCode(ic);
		return f;
	}

}
