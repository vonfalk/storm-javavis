#include "stdafx.h"
#include "Serialization.h"
#include "Str.h"
#include "Exception.h"

// TODO: Remove later!
#include "Core/Convert.h"

namespace storm {

	/**
	 * ObjIStream
	 */

	ObjIStream::ObjIStream(IStream *src) : from(src) {}

	static void checkBuffer(const Buffer &b) {
		if (!b.full())
			throw SerializationError(L"Not enough data.");
	}

	Byte ObjIStream::readByte() {
		GcPreArray<Byte, 1> d;
		Buffer b = from->read(emptyBuffer(d));
		checkBuffer(b);
		return d.v[0];
	}

	Int ObjIStream::readInt() {
		GcPreArray<Byte, 4> d;
		Buffer b = from->read(emptyBuffer(d));
		checkBuffer(b);
		Int r = Int(b[0]) << 24;
		r |= Int(b[1]) << 16;
		r |= Int(b[2]) << 8;
		r |= Int(b[3]) << 0;
		return r;
	}

	Nat ObjIStream::readNat() {
		GcPreArray<Byte, 4> d;
		Buffer b = from->read(emptyBuffer(d));
		checkBuffer(b);
		Nat r = Nat(b[0]) << 24;
		r |= Nat(b[1]) << 16;
		r |= Nat(b[2]) << 8;
		r |= Nat(b[3]) << 0;
		return r;
	}

	Long ObjIStream::readLong() {
		GcPreArray<Byte, 8> d;
		Buffer b = from->read(emptyBuffer(d));
		checkBuffer(b);
		Long r = Long(b[0]) << 56;
		r |= Long(b[1]) << 48;
		r |= Long(b[2]) << 40;
		r |= Long(b[3]) << 32;
		r |= Long(b[4]) << 24;
		r |= Long(b[5]) << 16;
		r |= Long(b[6]) << 8;
		r |= Long(b[7]) << 0;
		return r;
	}

	Word ObjIStream::readWord() {
		GcPreArray<Byte, 8> d;
		Buffer b = from->read(emptyBuffer(d));
		checkBuffer(b);
		Long r = Long(b[0]) << 56;
		r |= Long(b[1]) << 48;
		r |= Long(b[2]) << 40;
		r |= Long(b[3]) << 32;
		r |= Long(b[4]) << 24;
		r |= Long(b[5]) << 16;
		r |= Long(b[6]) << 8;
		r |= Long(b[7]) << 0;
		return r;
	}

	Float ObjIStream::readFloat() {
		Nat repr = readNat();
		Float r;
		memcpy(&r, &repr, sizeof(repr));
		return r;
	}

	Double ObjIStream::readDouble() {
		Word repr = readWord();
		Double r;
		memcpy(&r, &repr, sizeof(repr));
		return r;
	}


	/**
	 * ObjOStream
	 */

	static const Nat typeMask = 0x80000000;

	ObjOStream::ObjOStream(OStream *to) : to(to) {
		clearObjects();

		typeIds = new (this) Map<TObject *, Nat>();
		nextId = firstCustomId;

		// Insert the standard types inside 'ids', so we don't have to bother with them later.
		Engine &e = engine();
		typeIds->put((TObject *)StormInfo<Bool>::type(e), boolId);
		typeIds->put((TObject *)StormInfo<Byte>::type(e), byteId);
		typeIds->put((TObject *)StormInfo<Int>::type(e), intId);
		typeIds->put((TObject *)StormInfo<Nat>::type(e), natId);
		typeIds->put((TObject *)StormInfo<Long>::type(e), longId);
		typeIds->put((TObject *)StormInfo<Word>::type(e), wordId);
		typeIds->put((TObject *)StormInfo<Float>::type(e), floatId);
		typeIds->put((TObject *)StormInfo<Double>::type(e), doubleId);
		typeIds->put((TObject *)StormInfo<Str>::type(e), strId);
	}

	void ObjOStream::clearObjects() {
		MapBase *t = new (this) MapBase(StormInfo<TObject>::handle(engine()), StormInfo<Nat>::handle(engine()));
		objIds = (Map<Object *, Nat> *)t;
	}

	Nat ObjOStream::typeId(Type *type) {
		// TODO: We need to deal with containers somehow.
		TObject *t = (TObject *)type;
		Nat id = typeIds->get(t, nextId);
		if (id == nextId) {
			nextId++;
			id |= typeMask;
			typeIds->put(t, id);
		}
		return id;
	}

	serialize::Serialize ObjOStream::startValue(Type *t) {
		putHeader(t, true);
		return putTypeDesc(t) | serialize::body;
	}

	serialize::Serialize ObjOStream::startObject(Object *v) {
		Type *t = runtime::typeOf(v);
		putHeader(t, false);

		Nat count = objIds->count();
		Nat id = objIds->get(v, count);
		if (id != count) {
			// We have written this one before!
			writeNat(id);
			return serialize::none;
		}

		// Remember for the future.
		objIds->put(v, id);
		writeNat(id);

		// Output the actual type, it might differ from the formal type of the variable.
		writeNat(typeId(t) & ~typeMask);
		return putTypeDesc(t) | serialize::body;
	}

	void ObjOStream::putHeader(Type *t, Bool value) {
		if (depth++ == 0) {
			// This is the first object. Output its type!
			// Values have the highest bit set.
			Nat id = typeId(t) & ~typeMask;
			if (value)
				id |= typeMask;
			writeNat(id);
		}
	}

	serialize::Serialize ObjOStream::putTypeDesc(Type *t) {
		Nat id = typeId(t);
		if (id & typeMask) {
			// We need to output the type info for this type.
			id &= ~typeMask;
			typeIds->put((TObject *)t, id);
			// Type name, used for pretty-printing.
			runtime::typeName(t)->write(this);

			return serialize::members;
		} else {
			return serialize::none;
		}
	}

	void ObjOStream::member(Str *name, Type *t) {
		Nat id = typeId(t) & ~typeMask;
		if (runtime::isValue(t))
			id |= typeMask;
		writeNat(id);
		name->write(this);
	}

	void ObjOStream::endMembers() {
		// End of the members list.
		writeNat(0);
	}

	void ObjOStream::endBody() {
		if (depth == 0)
			throw InternalError(L"Calls to 'end' does not match calls to 'start' during serialization.");
		if (--depth == 0) {
			// All done!
			clearObjects();
		}
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
