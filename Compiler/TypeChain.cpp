#include "stdafx.h"
#include "TypeChain.h"
#include "Type.h"
#include "Core/Runtime.h"

namespace storm {

	TypeChain::TypeChain(Type *owner) : owner(owner), chain(null) {
		clearSuper();
	}

	TypeChain *TypeChain::superChain() const {
		if (chain->count == 1)
			return null;
		return chain->v[chain->count - 2];
	}

	Type *TypeChain::super() const {
		TypeChain *o = superChain();
		if (o)
			return o->owner;
		else
			return null;
	}

	void TypeChain::super(Type *o) {
		TypeChain *c = null;
		if (o)
			c = o->chain;
		super(c);
	}

	void TypeChain::super(TypeChain *n) {
		TypeChain *old = superChain();
		if (n == old)
			return;

		// if (old)
		// 	old->child.erase(this);
		// if (n)
		// 	n->child.insert(this);

		if (n)
			updateSuper(n);
		else
			clearSuper();
	}

	Bool TypeChain::isA(const TypeChain *o) const {
		return chain->count >= o->chain->count
			&& chain->v[o->chain->count - 1] == o;
	}

	Bool TypeChain::isA(const Type *o) const {
		return isA(o->chain);
	}

	Int TypeChain::distance(const TypeChain *o) const {
		if (!isA(o))
			return -1;
		return Int(chain->count - o->chain->count);
	}

	Int TypeChain::distance(const Type *o) const {
		return distance(o->chain);
	}

	static const GcType chainType = {
		GcType::tArray,
		null,
		null,
		sizeof(void *),
		1,
		{ 0 },
	};

	void TypeChain::updateSuper(const TypeChain *o) {
		nat count = o->chain->count;
		chain = runtime::allocArray<TypeChain *>(engine(), &chainType, count + 1);
		for (nat i = 0; i < count; i++)
			chain->v[i] = o->chain->v[i];
		chain->v[count] = this;

		notify();
	}

	void TypeChain::clearSuper() {
		chain = runtime::allocArray<TypeChain *>(engine(), &chainType, 1);
		chain->v[0] = this;

		notify();
	}

	void TypeChain::notify() const {
		// TODO: notify children!
	}

}
