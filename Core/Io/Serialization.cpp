#include "stdafx.h"
#include "Serialization.h"
#include "Str.h"
#include "Exception.h"

namespace storm {

	/**
	 * Object descriptions.
	 */

	SerializedMember::SerializedMember(Str *name, Type *type) : name(name), type(type) {}

	static void checkCtor(Type *t, FnBase *ctor) {
		// TODO: Check!
	}

	SerializedType::SerializedType(Type *t, FnBase *ctor)
		: type(t), readCtor(ctor), parent(null), members(new (engine()) Array<SerializedMember>()) {

		checkCtor(t, ctor);
	}

	SerializedType::SerializedType(Type *t, FnBase *ctor, SerializedType *parent)
		: type(t), readCtor(ctor), parent(parent), members(new (engine()) Array<SerializedMember>()) {

		checkCtor(t, ctor);
	}

	void SerializedType::add(Str *name, Type *type) {
		members->push(SerializedMember(name, type));
	}

	SerializedType::Cursor::Cursor() : type(null), pos(1) {}

	SerializedType::Cursor::Cursor(SerializedType *type) : type(type), pos(0) {
		if (!type->parent)
			pos++;
	}

	Type *SerializedType::Cursor::current() const {
		if (pos == 0)
			return type->parent->type;
		else
			return type->members->at(pos - 1).type;
	}


	/**
	 * ObjIStream
	 */

	ObjIStream::Desc::Desc(Byte flags, Nat parent, Str *name) : flags(flags), parent(parent) {
		memberNames = new (this) Array<Str *>();
		memberTypes = new (this) Array<Nat>();

		Type *t = runtime::fromIdentifier(name);
		if (!t)
			throw SerializationError(L"Unknown type: " + ::toS(demangleName(name)));
		const Handle &h = runtime::typeHandle(t);
		if (!h.serializedTypeFn)
			throw SerializationError(L"The type " + ::toS(demangleName(name)) + L" is not serializable.");

		info = (*h.serializedTypeFn)();

		// TOOD: Make sure that our view of the type matches with the one in the serialized stream.
	}


	ObjIStream::Cursor::Cursor() : desc(null), pos(0) {}

	ObjIStream::Cursor::Cursor(Desc *desc) : desc(desc), pos(0) {}

	Nat ObjIStream::Cursor::current() const {
		return desc->memberTypes->at(pos);
	}


	ObjIStream::ObjIStream(IStream *src) : from(src) {
		clearObjects();

		depth = new (this) Array<Cursor>();
		typeIds = new (this) Map<Nat, Desc *>();
	}

	Object *ObjIStream::readObject(Type *type) {
		// Note: the type represented by 'expectedId' may not always exactly match that of 'type'
		// since the types used at the root of deserialization do not need to be equal. However,
		// 'type' needs to be a parent of whatever we find as 'actual' later on, which is a subclass
		// of 'expected'.
		Nat expectedId = expectedType();
		Desc *expected = findInfo(expectedId);
		if ((expected->flags & typeInfo::classType) != typeInfo::classType)
			throw SerializationError(L"Expected a class type, but got a value type.");

		Nat objId = from->readNat();
		if (Object *old = objIds->get(objId, null))
			return old;

		// Read the actual type.
		Nat actualId = from->readNat();
		Desc *actual = findInfo(actualId);

		// Allocate the object and make sure the type is appropriate.
		Object *created = (Object *)runtime::allocObject(0, actual->info->type);
		if (!runtime::isA(created, type))
			throw SerializationError(L"Wrong type found during deserialization.");

		objIds->put(objId, created);

		// Find the parent classes, and push them on the stack so that we remember what we are
		// doing.
		{
			Desc *d = actual;
			while (d != null) {
				depth->push(Cursor(d));

				if (d->parent)
					d = findInfo(d->parent);
				else
					d = null;
			}
		}

		// Read everything by calling the constructor.
		ObjIStream *me = this;
		os::FnCall<void, 2> call = os::fnCall().add(created).add(me);
		actual->info->readCtor->callRaw(call, null, null);

		return created;
	}


	void ObjIStream::startCustom(StoredId id) {
		Nat expected = expectedType();
		if (expected != id)
			throw SerializationError(L"Type mismatch during deserialization!");

		depth->push(Cursor());
	}

	Nat ObjIStream::expectedType() {
		if (depth->empty()) {
			// First type. Read from the stream.
			return from->readNat();
		} else {
			Cursor &at = depth->last();
			Nat expected = at.current();
			depth->last().next();
			return expected;
		}
	}

	void ObjIStream::end() {
		if (depth->empty())
			throw SerializationError(L"Mismatched calls to startX during deserialization!");

		Cursor end = depth->last();
		if (end.more())
			throw SerializationError(L"Missing fields during serialization!");

		depth->pop();

		if (depth->empty()) {
			// Last one, clear the object cache.
			clearObjects();
		}
	}

	ObjIStream::Desc *ObjIStream::findInfo(Nat id) {
		Desc *result = typeIds->get(id, null);
		if (result)
			return result;

		Byte flags = from->readByte();
		Str *name = Str::read(from);
		Nat parent = from->readNat();

		result = new (this) Desc(flags, parent, name);

		// Members.
		for (Nat type = from->readNat(); type != 0; type = from->readNat()) {
			Str *name = Str::read(from);
			result->memberNames->push(name);
			result->memberTypes->push(type);
		}

		typeIds->put(id, result);
		return result;
	}

	void ObjIStream::clearObjects() {
		objIds = new (this) Map<Nat, Object *>();
	}


	/**
	 * ObjOStream
	 */

	static const Nat typeMask = 0x80000000;

	ObjOStream::ObjOStream(OStream *to) : to(to) {
		clearObjects();

		depth = new (this) Array<SerializedType::Cursor>();
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

		depth->push(SerializedType::Cursor());
	}

	Type *ObjOStream::start(SerializedType *type) {
		Type *r = type->type;
		if (depth->empty()) {
			// We're the root object, write a header.
			to->writeNat(typeId(type->type) & ~typeMask);
		} else {
			SerializedType::Cursor &at = depth->last();
			// Note: Will crash if we ever let a custom serialization call "regular" serialization (eg. containers).
			if (at.isParent())
				r = null;
			else
				r = at.current();
			at.next();
		}

		// Add a cursor to 'depth' to keep track of what we're doing!
		depth->push(SerializedType::Cursor(type));
		return r;
	}

	void ObjOStream::end() {
		if (depth->empty())
			throw SerializationError(L"Mismatched calls to startX during serialization!");

		SerializedType::Cursor end = depth->last();
		if (end.more())
			throw SerializationError(L"Missing fields during serialization!");

		depth->pop();

		if (depth->empty()) {
			// Last one, clear the object cache.
			clearObjects();
		}
	}

	void ObjOStream::writeInfo(SerializedType *t, Byte flags) {
		// Already written?
		Nat id = typeId(t->type);
		if ((id & typeMask) == 0)
			return;

		typeIds->put((TObject *)t->type, id & ~typeMask);

		to->writeByte(flags);
		runtime::typeIdentifier(t->type)->write(to);

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

	Str *demangleName(Str *name) {
		StrBuf *to = new (name) StrBuf();
		Bool addComma = false;
		for (Str::Iter i = name->begin(); i != name->end(); ++i) {
			Char ch = i.v();
			Bool comma = false;

			if (ch == Char(1u)) {
				Str::Iter j = i;
				if (++j != name->end())
					*to << S(".");
			} else if (ch == Char(2u)) {
				*to << S("(");
			} else if (ch == Char(3u)) {
				*to << S(")");
			} else if (ch == Char(4u)) {
				comma = true;
			} else if (ch == Char(5u)) {
				*to << S(" &");
				comma = true;
			} else {
				if (addComma)
					*to << S(", ");
				*to << ch;
			}

			addComma = comma;
		}
		return to->toS();
	}
}

