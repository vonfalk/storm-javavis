#include "stdafx.h"
#include "Function.h"
#include "Type.h"
#include "Engine.h"

namespace storm {

	Function::Function(Value result, const String &name, const vector<Value> &params)
		: NameOverload(name, params), result(result), source(null) {}

	Function::~Function() {
		delete source;
	}

	void *Function::pointer() {
		return ref().address();
	}

	code::RefSource &Function::ref() {
		if (!source) {
			assert(("Too early!", parent()));
			source = new code::RefSource(engine().arena, identifier());
			initRef(*source);
		}
		return *source;
	}

	void Function::output(wostream &to) const {
		to << result << " " << name << "(";
		join(to, params, L", ");
		to << ")";
	}


	NativeFunction::NativeFunction(Value result, const String &name, const vector<Value> &params, void *ptr)
		: Function(result, name, params), fnPtr(ptr) {}

	void NativeFunction::initRef(code::RefSource &ref) {
		ref.set(fnPtr);
	}

	/**
	 * Lazy function.
	 */

	LazyFunction::LazyFunction(Value result, const String &name, const vector<Value> &params)
		: Function(result, name, params) {}

	void LazyFunction::initRef(code::RefSource &ref) {}

}
