#include "stdafx.h"
#include "Serialization.h"

namespace storm {

	/**
	 * StoredField
	 */

	StoredField::StoredField(Str *name, StoredType *type, Nat offset)
		: name(name), type(type), offset(offset) {}

	void StoredField::deepCopy(CloneEnv *env) {
		cloned(name, env);
		cloned(type, env);
	}


	/**
	 * StoredType
	 */

	StoredType::StoredType(Type *t) : type(t) {
		fields = new (engine()) Array<StoredField>();
	}

	StoredType::StoredType(const StoredType &o) : type(o.type) {
		fields = new (this) Array<StoredField>(*o.fields);
	}

	void StoredType::deepCopy(CloneEnv *env) {
		for (Nat i = 0; i < fields->count(); i++) {
			fields->at(i).deepCopy(env);
		}
	}


	/**
	 * IObjStream
	 */

	IObjStream::IObjStream(IStream *src) : src(src) {}


	/**
	 * OObjStream
	 */

	OObjStream::OObjStream(OStream *dst) : dst(dst) {
		// These do not compile since 'Type' is not defined here, only forward declared.
		stored = new (this) Map<Type *, StoredType *>();
		ids = new (this) Map<Type *, Nat>();
		nextId = firstCustomId;
	}

	void OObjStream::write(StoredType *type, const void *value) {
		if (!stored->has(type->type)) {
			stored->put(type->type, type);

			// Serialize the type description. As a side-effect, this will assign ID:s to all
			// subtypes.
		}

		// If this is an object, we need to remember if we've stored it previously.

		for (Nat i = 0; i < type->count(); i++) {
			// Serialize each field according to the information in its type.
		}
	}

}
