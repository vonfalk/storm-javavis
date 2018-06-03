#include "stdafx.h"
#include "TypeChain.h"
#include "Type.h"
#include "Engine.h"
#include "Core/Runtime.h"
#include "Core/Str.h"

namespace storm {

	TypeChain::TypeChain(Type *owner) : owner(owner), chain(null), child(null) {
		if (engine().has(bootTemplates))
			child = new (engine()) WeakSet<TypeChain>();

		clearSuper();
	}

	void TypeChain::lateInit() {
		if (child)
			return;

		child = new (engine()) WeakSet<TypeChain>();
		TypeChain *super = superChain();
		if (super) {
			if (!super->child)
				super->lateInit();
			super->child->put(this);
			super->notify();
		}
	}

	TypeChain *TypeChain::superChain() const {
		if (chainCount() == 1)
			return null;
		return chainAt(chainCount() - 2);
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

		if (old && old->child)
			old->child->remove(this);
		if (n && n->child)
			n->child->put(this);

		if (n)
			updateSuper(n);
		else
			clearSuper();
	}

	Bool TypeChain::isA(const TypeChain *o) const {
		if (chain) {
			Nat c = o->chainCount();
			return chain->count >= c
				&& chain->v[c - 1] == o;
		} else {
			return o == this;
		}
	}

	Bool TypeChain::isA(const Type *o) const {
		return isA(o->chain);
	}

	Int TypeChain::distance(const TypeChain *o) const {
		if (!isA(o))
			return -1;
		return Int(chainCount() - o->chainCount());
	}

	Int TypeChain::distance(const Type *o) const {
		return distance(o->chain);
	}

	void TypeChain::updateSuper(const TypeChain *o) {
		nat count = o->chainCount();
		chain = runtime::allocArray<TypeChain *>(engine(), &pointerArrayType, count + 1);
		for (nat i = 0; i < count; i++)
			chain->v[i] = o->chainAt(i);
		chain->v[count] = this;

		notify();
	}

	void TypeChain::clearSuper() {
		chain = null;

		// chain = runtime::allocArray<TypeChain *>(engine(), &pointerArrayType, 1);
		// chain->v[0] = this;

		notify();
	}

	void TypeChain::notify() const {
		if (!child)
			return;

		WeakSet<TypeChain>::Iter i = child->iter();
		while (TypeChain *t = i.next())
			t->updateSuper(this);
	}

	TypeChain::Iter::Iter() : src() {}

	TypeChain::Iter::Iter(const Iter &o) : src(o.src) {}

	TypeChain::Iter::Iter(WeakSet<TypeChain> *src) : src(src->iter()) {}

	Type *TypeChain::Iter::next() {
		TypeChain *c = src.next();
		if (c)
			return c->owner;
		else
			return null;
	}

	TypeChain::Iter TypeChain::children() const {
		if (child)
			return Iter(child);
		else
			return Iter();
	}

}
