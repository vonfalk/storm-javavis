#include "stdafx.h"
#include "FnPtrTemplate.h"
#include "Engine.h"

namespace storm {

	FnPtrType::FnPtrType(const vector<Value> &v) : Type(L"FnPtr", typeClass, v) {
		setSuper(FnPtrBase::stormType(engine));
		matchFlags = matchNoInheritance;
	}

	void FnPtrType::lazyLoad() {}

	static Named *generateFnPtr(Par<NamePart> part) {
		if (part->params.size() < 1)
			return null;

		Engine &e = part->engine();
		Type *r = CREATE(FnPtrType, e, part->params);
		return r;
	}

	void addFnPtrTemplate(Par<Package> to) {
		Auto<Template> t = CREATE(Template, to, L"FnPtr", simpleFn(&generateFnPtr));
		to->add(t);
	}

	Type *fnPtrType(Engine &e, const vector<Value> &params) {
		Auto<Name> tName = CREATE(Name, e);
		tName->add(L"core");
		tName->add(L"FnPtr", params);
		Type *r = as<Type>(e.scope()->find(tName));
		assert(r, "The FnPtr type was not found!");
		return r;
	}


}
