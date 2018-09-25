#include "stdafx.h"
#include "Serialization.h"
#include "Str.h"

namespace storm {

	/**
	 * StoredField
	 */

	StoredField::StoredField(Str *name, Type *type, Nat offset)
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
	 * ObjIStream
	 */

	ObjIStream::ObjIStream(IStream *src) : src(src) {}


	/**
	 * ObjOStream
	 */

	ObjOStream::ObjOStream(OStream *to) : to(to) {
		stored = new (this) Map<TObject *, StoredType *>();
		ids = new (this) Map<TObject *, Nat>();
		nextId = firstCustomId;

		// Insert the standard types inside 'ids', so we don't have to bother with them later.
		Engine &e = engine();
		ids->put((TObject *)StormInfo<Byte>::type(e), byteId);
		ids->put((TObject *)StormInfo<Int>::type(e), intId);
		ids->put((TObject *)StormInfo<Nat>::type(e), natId);
		ids->put((TObject *)StormInfo<Long>::type(e), longId);
		ids->put((TObject *)StormInfo<Word>::type(e), wordId);
		ids->put((TObject *)StormInfo<Float>::type(e), floatId);
		ids->put((TObject *)StormInfo<Double>::type(e), doubleId);
		ids->put((TObject *)StormInfo<Str>::type(e), strId);
	}

	Nat ObjOStream::findId(Type *type) {
		// TODO: We need to deal with containers somehow.
		TObject *t = (TObject *)type;
		Nat id = ids->get(t, nextId);
		if (id == nextId) {
			nextId++;
			ids->put(t, id);
		}
		return id;
	}

	void ObjOStream::write(StoredType *type, const void *value) {
		TObject *t = (TObject *)type->type;
		if (!stored->has(t)) {
			stored->put(t, type);

			// Serialize the type description. As a side-effect, this will assign ID:s to all
			// subtypes.
			writeTypeDesc(type);
		}

		// If this is an object, we need to remember if we've stored it previously.

		for (Nat i = 0; i < type->count(); i++) {
			// Serialize each field according to the information in its type.
		}
	}

	void ObjOStream::writeTypeDesc(StoredType *type) {
		// TODO: I don't think we need the actual type here, we will receive it during
		// deserialization. However, it would be nice to write the name of the type here so that it
		// can be shown in diagnostic messages and during pretty printing of serialized output.

		// Number of types in here.
		writeNat(type->count());
		for (Nat i = 0; i < type->count(); i++) {
			writeField(type->at(i));
		}
	}

	void ObjOStream::writeField(StoredField f) {
		Nat typeId = findId(f.type);

		f.name->write(this);
		writeNat(typeId);
		writeNat(f.offset);
	}

	void ObjOStream::writeByte(Byte v) {
		GcPreArray<Byte, 1> d;
		d.v[0] = v;
		to->write(fullBuffer(d));
	}

	void ObjOStream::writeInt(Int v) {
		GcPreArray<Byte, 4> d;
		d.v[0] = Byte((v >> 24) & 0xFF);
		d.v[1] = Byte((v >> 16) & 0xFF);
		d.v[2] = Byte((v >>  8) & 0xFF);
		d.v[3] = Byte((v >>  0) & 0xFF);
		to->write(fullBuffer(d));
	}

	void ObjOStream::writeNat(Nat v) {
		GcPreArray<Byte, 4> d;
		d.v[0] = Byte((v >> 24) & 0xFF);
		d.v[1] = Byte((v >> 16) & 0xFF);
		d.v[2] = Byte((v >>  8) & 0xFF);
		d.v[3] = Byte((v >>  0) & 0xFF);
		to->write(fullBuffer(d));
	}

	void ObjOStream::writeLong(Long v) {
		GcPreArray<Byte, 8> d;
		d.v[0] = Byte((v >> 56) & 0xFF);
		d.v[1] = Byte((v >> 48) & 0xFF);
		d.v[2] = Byte((v >> 40) & 0xFF);
		d.v[3] = Byte((v >> 32) & 0xFF);
		d.v[4] = Byte((v >> 24) & 0xFF);
		d.v[5] = Byte((v >> 16) & 0xFF);
		d.v[6] = Byte((v >>  8) & 0xFF);
		d.v[7] = Byte((v >>  0) & 0xFF);
		to->write(fullBuffer(d));
	}

	void ObjOStream::writeWord(Word v) {
		GcPreArray<Byte, 8> d;
		d.v[0] = Byte((v >> 56) & 0xFF);
		d.v[1] = Byte((v >> 48) & 0xFF);
		d.v[2] = Byte((v >> 40) & 0xFF);
		d.v[3] = Byte((v >> 32) & 0xFF);
		d.v[4] = Byte((v >> 24) & 0xFF);
		d.v[5] = Byte((v >> 16) & 0xFF);
		d.v[6] = Byte((v >>  8) & 0xFF);
		d.v[7] = Byte((v >>  0) & 0xFF);
		to->write(fullBuffer(d));
	}

	void ObjOStream::writeFloat(Float v) {
		Nat repr;
		memcpy(&repr, &v, sizeof(v));
		writeNat(repr);
	}

	void ObjOStream::writeDouble(Double v) {
		Word repr;
		memcpy(&repr, &v, sizeof(v));
		writeWord(repr);
	}


}
