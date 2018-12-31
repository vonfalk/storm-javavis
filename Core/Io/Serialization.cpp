#include "stdafx.h"
#include "Serialization.h"
#include "Str.h"
#include "Exception.h"

namespace storm {

	/**
	 * Object descriptions.
	 */

	static void checkCtor(Type *t, FnBase *ctor) {
		// TODO: Check!
	}

	SerializedType::SerializedType(Type *t, FnBase *ctor)
		: type(t), readCtor(ctor), super(null) {

		checkCtor(t, ctor);
	}

	SerializedType::SerializedType(Type *t, FnBase *ctor, SerializedType *super)
		: type(t), readCtor(ctor), super(super) {

		checkCtor(t, ctor);
	}

	void SerializedType::toS(StrBuf *to) const {
		*to << S("Serialization info for ") << runtime::typeName(type) << S(":");
		if (super) {
			*to << S("\n  super: ");
			to->indent();
			super->toS(to);
			to->dedent();
		}
		*to << S("\n  constructor: ") << readCtor;
	}


	SerializedMember::SerializedMember(Str *name, Type *type) : name(name), type(type) {}

	SerializedStdType::SerializedStdType(Type *t, FnBase *ctor)
		: SerializedType(t, ctor), members(new (engine()) Array<SerializedMember>()) {}

	SerializedStdType::SerializedStdType(Type *t, FnBase *ctor, SerializedType *parent)
		: SerializedType(t, ctor, parent), members(new (engine()) Array<SerializedMember>()) {}

	void SerializedStdType::add(Str *name, Type *type) {
		members->push(SerializedMember(name, type));
	}

	void SerializedStdType::toS(StrBuf *to) const {
		SerializedType::toS(to);
		for (Nat i = 0; i < members->count(); i++)
			*to << S("\n  ") << members->at(i).name << S(": ") << runtime::typeName(members->at(i).type);
	}

	SerializedStdType::Cursor::Cursor() : type(null), pos(1) {}

	SerializedStdType::Cursor::Cursor(SerializedStdType *type) : type(type), pos(0) {
		if (!type->super)
			pos++;
	}

	Type *SerializedStdType::Cursor::current() const {
		if (pos == 0)
			return type->super->type;
		else
			return type->members->at(pos - 1).type;
	}


	/**
	 * ObjIStream
	 */

	ObjIStream::Member::Member(Str *name, Nat type) : name(name), type(type), read(0) {}

	ObjIStream::Member::Member(const Member &o, Int read) : name(o.name), type(o.type), read(read) {}


	ObjIStream::Desc::Desc(Byte flags, Nat parent, Str *name) : data(Nat(flags) << 24), parent(parent) {
		members = new (this) Array<Member>();

		Type *t = runtime::fromIdentifier(name);
		if (!t)
			throw SerializationError(L"Unknown type: " + ::toS(demangleName(name)));
		const Handle &h = runtime::typeHandle(t);
		if (!h.serializedTypeFn)
			throw SerializationError(L"The type " + ::toS(demangleName(name)) + L" is not serializable.");

		info = (*h.serializedTypeFn)();
	}

	ObjIStream::Desc::Desc(Byte flags, Type *type, FnBase *ctor) : data(Nat(flags) << 24), parent(0) {
		members = null;
		info = new (this) SerializedType(type, ctor);
	}

	Nat ObjIStream::Desc::findMember(Str *name) const {
		for (Nat i = 0; i < members->count(); i++)
			if (*members->at(i).name == *name)
				return i;
		return members->count();
	}


	ObjIStream::Cursor::Cursor() : desc(null), tmp(null), pos(0) {}

	ObjIStream::Cursor::Cursor(Desc *desc) : desc(desc), tmp(null), pos(0) {
		Nat entries = desc->storage();
		if (entries > 0) {
			// TODO: Maybe cache the array type somewhere?
			Engine &e = desc->engine();
			tmp = runtime::allocArray<Variant>(e, StormInfo<Variant>::handle(e).gcArrayType, entries);
		}
	}

	const ObjIStream::Member &ObjIStream::Cursor::current() const {
		return desc->members->at(pos);
	}

	void ObjIStream::Cursor::pushTemporary(const Variant &v) {
		tmp->v[tmp->filled++] = v;
	}


	template <class T, T (IStream::* readFn)()>
	static void CODECALL read(T *out, ObjIStream *from) {
		new (out) T((from->from->*readFn)());
		from->end();
	}

	static void readStr(Str **out, ObjIStream *from) {
		new (Place(out)) Str(from->from);
		from->end();
	}

	ObjIStream::ObjIStream(IStream *src) : from(src) {
		clearObjects();

		depth = new (this) Array<Cursor>();
		typeIds = new (this) Map<Nat, Desc *>();

#define ADD_BUILTIN(id, t)												\
		typeIds->put(id, new (this) Desc(								\
						typeInfo::valueType,							\
						StormInfo<t>::type(e),							\
						new (e) FnBase(address(&read<t, &IStream::read ## t>), null, false, null)))

		// Add built-in types here to avoid special cases later on.
		Engine &e = engine();
		ADD_BUILTIN(boolId, Bool);
		ADD_BUILTIN(byteId, Byte);
		ADD_BUILTIN(intId, Int);
		ADD_BUILTIN(natId, Nat);
		ADD_BUILTIN(longId, Long);
		ADD_BUILTIN(wordId, Word);
		ADD_BUILTIN(floatId, Float);
		ADD_BUILTIN(doubleId, Double);

#undef ADD_BUILTIN

		// String is a special case.
		typeIds->put(strId, new (this) Desc(typeInfo::classType, StormInfo<Str>::type(e),
														new (e) FnBase(address(&readStr), null, false, null)));
	}

	void ObjIStream::readValue(Type *type, PTR_GC out) {
		Info info = start();
		if (info.result.any()) {
			// TODO: Check type!
			info.result.moveValue(out);
			return;
		}

		Desc *expected = findInfo(info.expectedType);
		if (!expected->isValue())
			throw SerializationError(L"Expected a value type, but got a class type.");
		if (expected->info->type != type)
			throw SerializationError(L"Type mismatch. Expected " + ::toS(runtime::typeName(expected->info->type)) +
									L" but got " + ::toS(runtime::typeName(type)) + L".");

		readValueI(expected, out);
	}

	Object *ObjIStream::readObject(Type *type) {
		// Note: the type represented by 'expectedId' may not always exactly match that of 'type'
		// since the types used at the root of deserialization do not need to be equal. However,
		// 'type' needs to be a parent of whatever we find as 'actual' later on, which is a subclass
		// of 'expected'.
		Info info = start();
		if (info.result.any()) {
			// TODO: Check type!
			return (Object *)info.result.getObject();
		}

		Desc *expected = findInfo(info.expectedType);

		if (expected->isValue()) {
			// If this type has turned into a class type, just go ahead. There is no problem with
			// this as long as the members match.
			Object *created = (Object *)runtime::allocObject(0, expected->info->type);
			readValueI(expected, created);
			return created;
		}

		return readObjectI(expected, type);
	}

	void ObjIStream::readCustomValue(StoredId id, PTR_GC out) {
		Info info = start();
		Desc *expected = findInfo(info.expectedType);
		if (!expected->isValue())
			throw SerializationError(L"Expected a value type, but got a class type.");
		if (id != info.expectedType)
			throw SerializationError(L"Mismatch of built-in types!");

		if (info.result.any()) {
			// TODO: Check type!
			info.result.moveValue(out);
			return;
		}

		readValueI(expected, out);
	}

	Object *ObjIStream::readCustomObject(StoredId id) {
		Info info = start();
		Desc *expected = findInfo(info.expectedType);
		if (expected->isValue())
			throw SerializationError(L"Expected a class type, but got a value type.");
		if (id != info.expectedType)
			throw SerializationError(L"Mismatch of built-in types!");

		if (info.result.any()) {
			// TODO: Check type?
			return (Object *)info.result.getObject();
		}

		Object *created = (Object *)runtime::allocObject(0, expected->info->type);
		readValueI(expected, created);
		return created;
	}

	Variant ObjIStream::readObject(Nat typeId) {
		Desc *type = findInfo(typeId);
		if (type->isValue()) {
			Type *t = type->info->type;
			if (runtime::isValue(t)) {
				Variant v = Variant::uninitializedValue(t);
				readValueI(type, v.getValue());
				v.valueInitialized();
				return v;
			} else {
				// We support turning values into classes.
				Object *created = (Object *)runtime::allocObject(0, t);
				readValueI(type, created);
				return Variant(created);
			}
		} else {
			return Variant(readObjectI(type, type->info->type));
		}
	}

	void ObjIStream::readValueI(Desc *type, void *out) {
		// Find the parent classes and push them on the stack to keep track of what we're doing.
		Desc *d = type;
		while (d != null) {
			depth->push(Cursor(d));

			if (d->parent)
				d = findInfo(d->parent);
			else
				d = null;
		}

		// Call the constructor!
		ObjIStream *me = this;
		os::FnCall<void, 2> call = os::fnCall().add(out).add(me);
		type->info->readCtor->callRaw(call, null, null);
	}

	Object *ObjIStream::readObjectI(Desc *expected, Type *t) {
		// Did we encounter this instance before?
		Nat objId = from->readNat();
		if (Object *old = objIds->get(objId, null)) {
			if (!runtime::isA(old, t))
				throw SerializationError(L"Wrong type found during deserialization.");
			return old;
		}

		// Read the actual type.
		Nat actualId = from->readNat();
		Desc *actual = findInfo(actualId);

		// Make sure the type we're about to create is appropriate.
		if (!runtime::isA(actual->info->type, t))
			throw SerializationError(L"Wrong type found during deserialization.");

		// Allocate the object and deserialize by calling the constructor, just like we do with value types.
		Object *created = (Object *)runtime::allocObject(0, actual->info->type);
		objIds->put(objId, created);

		readValueI(actual, created);

		return created;
	}

	ObjIStream::Info ObjIStream::start() {
		Info r = { 0, Variant() };

		if (depth->empty()) {
			// First type. Read its id from the stream.
			r.expectedType = from->readNat();
			return r;
		}

		// Some other type. Examine what we expect to read.
		Cursor &at = depth->last();
		assert(!at.customDesc(), L"We don't handle serializing other types inside custom serialization yet!");

		// Process objects until we find something we can return!
		while (true) {
			const Member &expected = at.current();
			at.next();

			if (expected.read < 0) {
				// Read one object into temporary storage and continue.
				at.pushTemporary(readObject(expected.type));
			} else if (expected.read > 0) {
				r.expectedType = expected.type;
				r.result = at.temporary(Nat(expected.read - 1));
				return r;
			} else {
				// Read it now!
				r.expectedType = expected.type;
				return r;
			}
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
			result->members->push(Member(name, type));
		}

		validate(result);

		typeIds->put(id, result);
		return result;
	}

	void ObjIStream::validate(Desc *stream) {
		SerializedStdType *our = as<SerializedStdType>(stream->info);
		if (!our) {
			// Custom type description. Nothing to validate!
			stream->members = null;
			return;
		}

		// Note: We check the parent type when reading objects. Otherwise, we need quite a bit of
		// bookkeeping to know when to validate parents of all types unless we want to check all
		// types every time a new type is found in the stream.

		// Note: We can not check the types of members here, as all member types are not necessarily
		// known at this point. This is instead done when 'readXxx' is called.

		// Check the members we need to find, and match them to the members in the stream. During
		// the process, figure out how to use the temporary storage to store some members there if
		// necessary.
		Nat streamPos = 0;
		Map<Str *, Member> *tempPos = new (this) Map<Str *, Member>();
		// Note: Original count, not updated when we grow the array. This is intentional in order to
		// not catch the duplicate entries referring to temporary storage!
		Nat memberCount = stream->members->count();
		for (Nat i = 0; i < our->members->count(); i++) {
			const SerializedMember &ourMember = our->members->at(i);

			// Look until we find it in the stream, saving intermediate members to temporary storage.
			while (streamPos < memberCount) {
				Member &m = stream->members->at(streamPos);
				if (*m.name == *ourMember.name)
					break;

				// Store it in temporary storage and remember its location (indexed from 1).
				m.read = -1;
				tempPos->put(m.name, Member(m, tempPos->count() + 1));
				streamPos++;
			}

			if (streamPos < memberCount) {
				// If we do, we can read it without temporary storage.
				stream->members->at(streamPos).read = 0;
				streamPos++;
			} else if (tempPos->has(ourMember.name)) {
				// If we skipped it earlier, read it from temporary storage.
				stream->members->push(tempPos->get(ourMember.name));
			} else {
				// Otherwise, an error.

				// TODO: When we have support for default initialization, take that into consideration here!
				throw SerializationError(L"The member " + ::toS(ourMember.name) + L", required for type " +
										::toS(runtime::typeName(our->type)) + L", is not present in the stream.");
			}
		}

		// Remember the number of bytes required in temporary storage.
		stream->storage(tempPos->count());

		// PLN(L"Members of " << runtime::typeName(our->type));
		// for (Nat i = 0; i < stream->members->count(); i++) {
		// 	Member t = stream->members->at(i);

		// 	PLN(L"  " << t.name << L", " << t.type << L", " << t.read);
		// }
		// PLN(L"  (" << stream->storage() << L" temporary entries required)");
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

		depth = new (this) Array<SerializedStdType::Cursor>();
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
			while (expectedDesc && expectedDesc->type != expected)
				expectedDesc = expectedDesc->super;
			if (!expectedDesc)
				throw SerializationError(L"The provided type description does not match the serialized object.");

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
		// TODO: Revise this! We could probably make it a special case of 'start' below.
		startCustom(StoredId(typeId(type)));
	}

	void ObjOStream::startCustom(StoredId id) {
		if (depth->empty()) {
			to->writeNat(id & ~typeMask);
		} else {
			depth->last().next();
		}

		depth->push(SerializedStdType::Cursor());
	}

	Type *ObjOStream::start(SerializedType *type) {
		Type *r = type->type;
		if (depth->empty()) {
			// We're the root object, write a header.
			to->writeNat(typeId(type->type) & ~typeMask);
		} else {
			SerializedStdType::Cursor &at = depth->last();
			// Note: Will crash if we ever let a custom serialization call "regular" serialization (eg. containers).
			if (at.isParent())
				r = null;
			else
				r = at.current();
			at.next();
		}

		// Add a cursor to 'depth' to keep track of what we're doing!
		if (SerializedStdType *t = as<SerializedStdType>(type)) {
			depth->push(SerializedStdType::Cursor(t));
		} else {
			// Custom type. We don't know about it.
			depth->push(SerializedStdType::Cursor());
		}
		return r;
	}

	void ObjOStream::end() {
		if (depth->empty())
			throw SerializationError(L"Mismatched calls to startX during serialization!");

		SerializedStdType::Cursor end = depth->last();
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

		if (t->super) {
			to->writeNat(typeId(t->super->type) & ~typeMask);
		} else {
			to->writeNat(endId);
		}

		if (SerializedStdType *s = as<SerializedStdType>(t)) {
			// Members.
			for (Nat i = 0; i < s->members->count(); i++) {
				const SerializedMember &member = s->members->at(i);

				Nat id = typeId(member.type);
				to->writeNat(id & ~typeMask);
				member.name->write(to);
			}

			// End of members.
			to->writeNat(endId);
		} else {
			// Note: We need to indicate the absence of members in the flags.
			assert(false, L"Not writing members is not yet supported!");
		}
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

