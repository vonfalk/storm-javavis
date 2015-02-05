#include "stdafx.h"
#include "ArrayTemplate.h"
#include "Array.h"
#include "Function.h"
#include "Exception.h"
#include "TypeCtor.h"

namespace storm {

	static void CODECALL createClass(void *mem) {
		new (mem) Array<Auto<Object>>();
	}

	static void CODECALL pushClass(Array<Auto<Object>> *to, Par<Object> o) {
		to->push(o);
	}

	static Object *CODECALL getClass(Array<Auto<Object>> *to, Nat id) {
		return to->at(id).ret();
	}

	static Named *generateArray(Par<NamePart> part) {
		if (part->params.size() != 1)
			return null;

		Engine &e = part->engine();

		const Value &type = part->params[0];
		Type *r = CREATE(ArrayType, e, type);
		r->setSuper(ArrayBase::type(e));
		return r;
	}

	Template *arrayTemplate(Engine &e) {
		return CREATE(Template, e, L"Array", simpleFn(&generateArray));
	}

	ArrayType::ArrayType(const Value &param) : Type(L"Array", typeClass, valList(1, param)), param(param) {}

	void ArrayType::lazyLoad() {
		if (param.isClass())
			loadClassFns();
		else
			throw InternalError(L"The type " + ::toS(param) + L" is not supported for arrays yet.");
	}

	void ArrayType::loadClassFns() {
		Engine &e = engine;
		Value t = Value::thisPtr(this);
		add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createClass))));
		add(steal(nativeFunction(e, Value(), L"<<", valList(2, t, param), address(&pushClass))));
		add(steal(nativeFunction(e, param, L"[]", valList(2, t, Value(natType(e))), address(&getClass))));
	}

}
