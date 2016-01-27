#include "stdafx.h"
#include "BSContents.h"
#include "BSClass.h"

namespace storm {

	bs::Contents::Contents() :
		types(CREATE(ArrayP<Type>, this)),
		functions(CREATE(ArrayP<FunctionDecl>, this)),
		threads(CREATE(ArrayP<NamedThread>, this)),
		templates(CREATE(MAP_PP(Str, TemplateAdapter), this)) {}

	void bs::Contents::add(Par<Type> t) {
		types->push(t);
	}

	void bs::Contents::add(Par<FunctionDecl> t) {
		functions->push(t);
	}

	void bs::Contents::add(Par<NamedThread> t) {
		threads->push(t);
	}

	void bs::Contents::add(Par<Template> t) {
		Auto<Str> k = CREATE(Str, this, t->name);
		if (templates->has(k)) {
			Auto<TemplateAdapter> &found = templates->get(k);
			found->add(t);
		} else {
			Auto<TemplateAdapter> adapter = CREATE(TemplateAdapter, this, k);
			adapter->add(t);
			templates->put(k, adapter);
		}
	}

	void bs::Contents::setScope(const Scope &scope) {
		for (nat i = 0; i < types->count(); i++) {
			if (bs::Class *c = as<bs::Class>(types->at(i).borrow())) {
				c->setScope(scope);
			}
		}
	}

}
