#include "stdafx.h"
#include "TypeChain.h"
#include "Type.h"

namespace storm {

	TypeChain::TypeChain(Type *owner) : chain(null), owner(owner) {
		clearSuper();
	}

	TypeChain::~TypeChain() {
		if (TypeChain *s = superChain())
			s->child.erase(this);

		vector<TypeChain *> c(child.begin(), child.end());
		for (nat i = 0; i < c.size(); i++)
			c[i]->super(null);
		delete []chain;
	}

	TypeChain *TypeChain::superChain() const {
		if (count == 1)
			return null;
		return chain[count - 2];
	}

	Type *TypeChain::super() const {
		if (count == 1)
			return null;
		return chain[count - 2]->owner;
	}

	void TypeChain::super(Type *o) {
		TypeChain *n = null;
		if (o)
			n = &o->chain;

		TypeChain *old = superChain();
		if (n == old)
			return;

		if (old)
			old->child.erase(this);
		if (n)
			n->child.insert(this);

		if (n)
			updateSuper(*n);
		else
			clearSuper();
	}

	bool TypeChain::isA(TypeChain *o) const {
		return count >= o->count
			&& chain[o->count - 1] == o;
	}

	bool TypeChain::isA(Type *o) const {
		return isA(&o->chain);
	}

	void TypeChain::updateSuper(const TypeChain &o) {
		delete []chain;

		count = o.count + 1;
		chain = new TypeChain*[count];
		for (nat i = 0; i < o.count; i++)
			chain[i] = o.chain[i];
		chain[o.count] = this;

		notify();
	}

	void TypeChain::clearSuper() {
		delete []chain;

		chain = new TypeChain*[1];
		chain[0] = this;
		count = 1;

		notify();
	}

	void TypeChain::notify() const {
		ChildSet::const_iterator i, end = child.end();
		for (i = child.begin(); i != end; ++i) {
			TypeChain *c = *i;
			c->updateSuper(*this);
		}
	}

	vector<Type*> TypeChain::children() const {
		vector<Type*> r(child.size());

		ChildSet::const_iterator i, end = child.end();
		nat to = 0;
		for (i = child.begin(); i != end; ++i, to++)
			r[to] = (*i)->owner;

		return r;
	}

}
