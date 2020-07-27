#include "stdafx.h"
#include "ReplaceContext.h"
#include "ExactPart.h"
#include "Engine.h"

namespace storm {

	ReplaceContext::ReplaceContext() {
		typeEq = new (this) Map<Type *, Type *>();
	}

	Bool ReplaceContext::same(Type *a, Type *b) {
		return typeEq->get(a, a) == typeEq->get(b, b);
	}

	Type *ReplaceContext::normalize(Type *t) {
		return typeEq->get(t, t);
	}

	Value ReplaceContext::normalize(Value v) {
		v.type = typeEq->get(v.type, v.type);
		return v;
	}

	void ReplaceContext::buildTypeEquivalence(NameSet *oldRoot, NameSet *newRoot) {
		buildTypeEq(oldRoot, oldRoot, newRoot);
	}

	void ReplaceContext::buildTypeEq(NameSet *oldRoot, NameSet *currOld, NameSet *currNew) {
		for (NameSet::Iter i = currNew->begin(), end = currNew->end(); i != end; ++i) {
			Type *n = as<Type>(i.v());
			if (!n)
				continue;

			ExactPart *p = new (this) ExactPart(n->name, resolveParams(oldRoot, n->params));
			Type *old = as<Type>(currOld->find(p, Scope(currOld)));
			if (!old || old == n)
				continue;

			typeEq->put(n, old);
			buildTypeEq(oldRoot, old, n);
		}
	}

	Array<Value> *ReplaceContext::resolveParams(NameSet *root, Array<Value> *params) {
		if (params->empty())
			return params;

		params = new (this) Array<Value>(*params);
		for (Nat i = 0; i < params->count(); i++)
			params->at(i) = resolveParam(root, params->at(i));

		return params;
	}

	Value ReplaceContext::resolveParam(NameSet *root, Value param) {
		if (param == Value())
			return param;

		// Do we know of this parameter already?
		if (Type *t = typeEq->get(param.type, null)) {
			param.type = t;
			return t;
		}

		// No, we need to look it up ourselves. It has probably not been processed yet.
		Array<Named *> *prev = new (this) Array<Named *>();
		Named *current = param.type;
		while (current && current == root) {
			prev->push(current);
			current = as<Named>(current->parent());
		}

		NameSet *back = as<NameSet>(current);

		// No luck.
		if (!back)
			return param;

		// Now, try to match each name in 'prev' inside 'root'.
		while (prev->any() && back) {
			Named *n = prev->last();
			ExactPart *p = new (this) ExactPart(n->name, resolveParams(root, n->params));
			back = as<NameSet>(back->find(p, Scope(back)));

			prev->pop();
		}

		if (Type *t = as<Type>(back))
			param.type = t;
		return param;
	}


	ReplaceTasks::ReplaceTasks() {
		replaceMap = new ObjMap<Named>(engine().gc);
	}

	ReplaceTasks::~ReplaceTasks() {
		delete replaceMap;
	}

	void ReplaceTasks::replace(Named *old, Named *with) {
		replaceMap->put(old, with);
	}

}
