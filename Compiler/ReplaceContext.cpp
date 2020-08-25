#include "stdafx.h"
#include "ReplaceContext.h"
#include "ExactPart.h"
#include "Engine.h"
#include "VTable.h"

namespace storm {

	ReplaceContext::ReplaceContext() {
		typeEq = new (this) Map<Type *, Type *>();
	}

	Bool ReplaceContext::same(Type *a, Type *b) {
		return typeEq->get(a, a) == typeEq->get(b, b);
	}

	Type *ReplaceContext::normalize(Type *t) {
		if (t)
			return typeEq->get(t, t);
		else
			return null;
	}

	Value ReplaceContext::normalize(Value v) {
		v.type = typeEq->get(v.type, v.type);
		return v;
	}

	void ReplaceContext::buildTypeEquivalence(NameSet *oldRoot, NameSet *newRoot) {
		buildTypeEq(oldRoot, oldRoot, newRoot);
	}

	void ReplaceContext::addEq(Type *oldType, Type *newType) {
		typeEq->put(newType, oldType);
	}

	void ReplaceContext::buildTypeEq(NameSet *oldRoot, NameSet *currOld, NameSet *currNew) {
		currNew->forceLoad();
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
		replaceMap = new RawObjMap(engine().gc);
		vtableMap = new RawObjMap(engine().gc);
	}

	ReplaceTasks::~ReplaceTasks() {
		delete replaceMap;
		delete vtableMap;
	}

	void ReplaceTasks::replace(Named *old, Named *with) {
		replaceMap->put(old, with);
	}

	void ReplaceTasks::replace(const Handle *old, const Handle *with) {
		replaceMap->put((void *)old, (void *)with);
	}

	void ReplaceTasks::replace(const GcType *old, const GcType *with) {
		replaceMap->put((void *)old, (void *)with);
	}

	void ReplaceTasks::replace(VTable *old, VTable *with) {
		void *o = (void *)old->pointer();
		void *n = (void *)with->pointer();
		size_t offset = vtable::allocOffset();
		vtableMap->put((byte *)o - offset, (byte *)n - offset);
	}

	class ReplaceWalker : public PtrWalker {
	public:
		ReplaceWalker(RawObjMap *replace, RawObjMap *vtables) : replace(replace), vtables(vtables) {
			flags = fObjects | fExactRoots | fClearWatch;
			// TODO: If we are rewriting other objects, we need to add fAmbiguousRoots as well.
		}

		RawObjMap *replace;
		RawObjMap *vtables;
		bool foundHeader;

		virtual void prepare() {
			// Now, objects don't move anymore and we can sort the array for good lookup performance!
			replace->sort();
			vtables->sort();
		}

		virtual bool checkRoot(GcRoot *root) {
			// Don't modify the maps we're working with now!
			return !replace->hasRoot(root)
				&& !vtables->hasRoot(root);
		}

		virtual void object(RootObject *obj) {
			// Check the vtable.
			void *vt = (void *)vtable::from(obj);
			size_t offset = vtable::allocOffset();
			if (void *r = vtables->find((byte *)vt - offset)) {
				// Need to go through the write() interface.
				// vtable::set((byte *)r + offset, obj);
				write((void **)obj, (byte *)r + offset);
			}

			foundHeader = false;

			// Check all pointers.
			PtrWalker::object(obj);
		}

		virtual void fixed(void *obj) {
			foundHeader = false;
			PtrWalker::fixed(obj);
		}

		virtual void array(void *obj) {
			foundHeader = false;
			PtrWalker::fixed(obj);
		}

		virtual void header(GcType **ptr) {
			// Only scan the first header of each object (i.e. don't scan the 'myGcType' inside Type objects).
			if (foundHeader)
				return;
			foundHeader = true;

			if (void *r = replace->find(*ptr)) {
				// *ptr = (GcType *)r;
				write((void **)ptr, r);
			}
		}

		virtual void exactPointer(void **ptr) {
			if (void *r = replace->find(*ptr)) {
				// *ptr = r;
				write(ptr, r);
			}
		}

		virtual void ambiguousPointer(void **ptr) {
			// TODO. We need more information on the objects to replace in this case.
		}
	};

	void ReplaceTasks::apply() {
		ReplaceWalker walker(replaceMap, vtableMap);
		engine().gc.walk(walker);
	}

}
