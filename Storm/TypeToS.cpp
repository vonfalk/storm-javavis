#include "stdafx.h"
#include "TypeToS.h"
#include "Type.h"
#include "Engine.h"
#include "Shared/StrBuf.h"

namespace storm {

	TypeToS::TypeToS(Type *owner) :
		Function(Value::thisPtr(Str::stormType(engine())), L"toS", vector<Value>(1, Value::thisPtr(owner))) {

		Auto<FnPtr<CodeGen *>> fn = memberWeakPtr(engine(), this, &TypeToS::generateCode);
		setCode(steal(CREATE(LazyCode, this, fn)));
	}

	static vector<code::Value> z(const code::Value &a, const code::Value &b) {
		vector<code::Value> r;
		r.push_back(a);
		r.push_back(b);
		return r;
	}

	static bool sortVars(const Auto<TypeVar> &a, const Auto<TypeVar> &b) {
		return a->name < b->name;
	}

	CodeGen *TypeToS::generateCode() {
		using namespace code;

		Auto<CodeGen> g = CREATE(CodeGen, this, runOn());
		Value strBufType = Value::thisPtr(StrBuf::stormType(engine()));
		Value strType = Value::thisPtr(Str::stormType(engine()));
		Value natType(storm::natType(engine()));

		Variable thisPtr = g->frame.createPtrParam();
		g->to << prolog();

		// Create the StrBuf.
		Variable strBuf = allocObject(g, strBufType.type->defaultCtor(), vector<code::Value>());

		Auto<Function> addChar = steal(strBufType.type->findWCpp(L"addChar", valList(2, strBufType, natType))).as<Function>();
		Auto<Function> addStr = steal(strBufType.type->findWCpp(L"add", valList(2, strBufType, strType))).as<Function>();

		// Add leading {
		addChar->autoCall(g, z(strBuf, natConst('{')), steal(CREATE(CodeResult, this)));

		// Add all variables: (todo: super-classes as well!)
		vector<Auto<TypeVar>> vars = params[0].type->variables();
		sort(vars.begin(), vars.end(), &sortVars);

		for (nat i = 0; i < vars.size(); i++) {
			if (i != 0) {
				addChar->autoCall(g, z(strBuf, intConst(' ')), steal(CREATE(CodeResult, this)));
			}

			TypeVar *v = vars[i].borrow();
			Value type = v->varType;

			Auto<Name> name = CREATE(Name, this, L"toS", valList(1, type));
			Auto<Function> f = steal(storm::findW(type.type, name)).as<Function>();
			if (!f)
				f = steal(storm::findW(engine().package(L"core"), name)).as<Function>();

			if (!f) {
				addChar->autoCall(g, z(strBuf, intConst('?')), steal(CREATE(CodeResult, this)));
				continue;
			}
			Auto<CodeResult> r = CREATE(CodeResult, this, strType, g->block);

			g->to << mov(ptrA, thisPtr);
			if (type.isClass()) {
				g->to << mov(ptrA, ptrRel(ptrA, v->offset()));
				f->autoCall(g, vector<code::Value>(1, ptrA), r);
			} else {
				g->to << add(ptrA, intPtrConst(v->offset()));
				f->autoCall(g, vector<code::Value>(1, ptrA), r);
			}

			addStr->autoCall(g, z(strBuf, r->location(g).var()), steal(CREATE(CodeResult, this)));
		}

		// Add trailing }
		addChar->autoCall(g, z(strBuf, intConst('}')), steal(CREATE(CodeResult, this)));


		// Extract the actual string.
		Auto<Function> toS = steal(strBufType.type->findWCpp(L"toS", valList(1, strBufType))).as<Function>();
		Auto<CodeResult> result = CREATE(CodeResult, this, strType, g->block);
		toS->autoCall(g, vector<code::Value>(1, strBuf), result);
		g->to << code::addRef(result->location(g).var());
		g->to << mov(ptrA, result->location(g).var());
		g->to << epilog();
		g->to << ret(retPtr());

		return g.ret();
	}

}
