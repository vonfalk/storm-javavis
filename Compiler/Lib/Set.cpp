#include "stdafx.h"
#include "Set.h"
#include "Core/Str.h"
#include "Core/Set.h"
#include "Core/WeakSet.h"

namespace storm {

	Type *createSet(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value k = params->at(0);
		if (k.ref)
			return null;

		return new (params) SetType(name, k.type);
	}

	SetType::SetType(Str *name, Type *k) : Type(name, typeClass), k(k) {
		params = new (engine) Array<Value>(Value(k));

		setSuper(SetBase::stormType(engine));
	}

	Type *createWeakSet(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value k = params->at(0);
		if (k.ref)
			return null;

		// Disallow non-actors.
		if (!k.type->isA(TObject::stormType(name->engine())))
			return null;

		return new (params) WeakSetType(name, k.type);
	}

	WeakSetType::WeakSetType(Str *name, Type *k) : Type(name, typeClass), k(k) {
		params = new (engine) Array<Value>(Value(k));

		setSuper(WeakSetBase::stormType(engine));
	}

}
