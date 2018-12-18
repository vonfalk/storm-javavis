#include "stdafx.h"
#include "Serialization.h"
#include "Str.h"
#include "Exception.h"

namespace storm {

	/**
	 * Object descriptions.
	 */

	SerializedMember::SerializedMember(Str *name, Type *type) : name(name), type(type) {}

	SerializedType::SerializedType(Type *t)
		: type(t), parent(null), members(new (engine()) Array<SerializedMember>()) {}

	SerializedType::SerializedType(Type *t, SerializedType *parent)
		: type(t), parent(parent), members(new (engine()) Array<SerializedMember>()) {}

	void SerializedType::add(Str *name, Type *type) {
		members->push(SerializedMember(name, type));
	}

	Cursor::Cursor() : type(null), pos(1) {}

	Cursor::Cursor(SerializedType *type) : type(type), pos(0) {
		if (!type->parent)
			pos++;
	}

	Type *Cursor::current() const {
		if (pos == 0)
			return type->parent->type;
		else
			return type->members->at(pos - 1).type;
	}


	/**
	 * ObjIStream
	 */

	ObjIStream::ObjIStream(IStream *src) : from(src) {}


	/**
	 * ObjOStream
	 */

	static const Nat typeMask = 0x80000000;

	ObjOStream::ObjOStream(OStream *to) : to(to) {
		clearObjects();

		depth = new (this) Array<Cursor>();
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

	Bool ObjOStream::startValue(SerializedType *type) {
		Type *expected = start(type);
		if (expected) {
			// We can assume that the actual type is exactly what we're expecting since value types
			// are sliced. Therefore, we don't need to search for the proper type as we need to do
			// for classes.
			assert(type->type == expected, L"Internal serialization error.");
		}

		writeInfo(type, typeInfo::valueType);
		return true;
	}

	Bool ObjOStream::startObject(SerializedType *type, Object *v) {
		Type *expected = start(type);
		if (expected) {
			// This is the start of a new type.

			// Find the expected type from the description. It should be a direct or indirect parent!
			SerializedType *expectedDesc = type;
			while (expected && expectedDesc->type != expected)
				expectedDesc = expectedDesc->parent;
			writeInfo(expectedDesc, typeInfo::classType);

			// Now, the reader knows what we're talking about. Now we can bother with references...
			Nat objId = objIds->get(v, objIds->count());
			to->writeNat(objId);

			if (objId != objIds->count()) {
				// Already existing object?

				// Discard the serialization step for that. If we call 'end', an exception will be thrown.
				depth->pop();
				return false;
			}

			// New object. Write its actual type.
			to->writeNat(typeId(type->type) & ~typeMask);
			objIds->put(v, objId);
		} else {
			// This is the parent of an object we're already serializing. We don't need any
			// additional headers for that, just possibly the class description.
		}

		writeInfo(type, typeInfo::classType);
		return true;
	}

	void ObjOStream::startCustom(Type *type) {
		startCustom(StoredId(typeId(type)));
	}

	void ObjOStream::startCustom(StoredId id) {
		if (depth->empty()) {
			to->writeNat(id & ~typeMask);
		} else {
			depth->last().next();
		}

		depth->push(Cursor());
	}

	Type *ObjOStream::start(SerializedType *type) {
		Type *r = type->type;
		if (depth->empty()) {
			// We're the root object, write a header.
			to->writeNat(typeId(type->type) & ~typeMask);
		} else {
			Cursor &at = depth->last();
			// Note: Will crash if we ever let a custom serialization call "regular" serialization (eg. containers).
			if (at.isParent())
				r = null;
			else
				r = at.current();
			at.next();
		}

		// Add a cursor to 'depth' to keep track of what we're doing!
		depth->push(Cursor(type));
		return r;
	}

	void ObjOStream::end() {
		if (depth->empty())
			throw SerializationError(L"Mismatched calls to startX during serialization!");

		Cursor end = depth->last();
		if (end.more())
			throw SerializationError(L"Missing fields during serialization!");

		depth->pop();

		if (depth->empty()) {
			// Last one, clear the object cache.
			objIds->clear();
		}
	}

	void ObjOStream::writeInfo(SerializedType *t, Byte flags) {
		// Already written?
		Nat id = typeId(t->type);
		if ((id & typeMask) == 0)
			return;

		typeIds->put((TObject *)t->type, id & ~typeMask);

		to->writeByte(flags);
		// TODO: We want to output a mangled name of the class!
		runtime::typeName(t->type)->write(to);

		if (t->parent) {
			to->writeNat(typeId(t->parent->type) & ~typeMask);
		} else {
			to->writeNat(endId);
		}

		// Members.
		for (Nat i = 0; i < t->members->count(); i++) {
			const SerializedMember &member = t->members->at(i);

			Nat id = typeId(member.type);
			to->writeNat(id & ~typeMask);
			member.name->write(to);
		}

		// End of members.
		to->writeNat(endId);
	}

}

