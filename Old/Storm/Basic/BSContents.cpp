#include "stdafx.h"
#include "BSContents.h"
#include "BSClass.h"
#include "Exception.h"

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

	void bs::Contents::add(Par<TObject> o) {
		if (Type *t = as<Type>(o.borrow()))
			add(Par<Type>(t));
		else if (FunctionDecl *fn = as<FunctionDecl>(o.borrow()))
			add(Par<FunctionDecl>(fn));
		else if (NamedThread *nt = as<NamedThread>(o.borrow()))
			add(Par<NamedThread>(nt));
		else if (Template *t = as<Template>(o.borrow()))
			add(Par<Template>(t));
		else
			throw InternalError(L"add for Content does not expect " + o->myType->identifier());
	}

}
