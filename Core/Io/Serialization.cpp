#include "stdafx.h"
#include "Serialization.h"
#include "Str.h"
#include "Exception.h"

namespace storm {
	namespace serialize {

		/**
		 * Object descriptions.
		 */

		TypeMember::TypeMember(Str *name, Type *type) : name(name), type(type) {}

		TypeDesc::TypeDesc(Type *t)
			: type(t), parent(null), members(new (engine()) Array<TypeMember>()) {}

		TypeDesc::TypeDesc(Type *t, TypeDesc *parent)
			: type(t), parent(parent), members(new (engine()) Array<TypeMember>()) {}

		void TypeDesc::add(Str *name, Type *type) {
			members->push(TypeMember(name, type));
		}

		Cursor::Cursor() : type(null), pos(0) {}

		Cursor::Cursor(TypeDesc *type) : type(type), pos(0) {}

	}

	using namespace serialize;


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

	Bool ObjOStream::startValue(TypeDesc *type) {
		// TODO!
		return true;
	}

	Bool ObjOStream::startObject(TypeDesc *type, Object *v) {
		// TODO: What about serializing the parent type?

		Type *expected = start(type);
		// Find the expected type from the description. It should be a direct or indirect parent!
		TypeDesc *expectedDesc = type;
		while (expected && expectedDesc->type != expected)
			expectedDesc = expectedDesc->parent;
		writeInfo(expectedDesc, typeInfo::classType);

		// Now, the reader knows what we're talking about. Now we can bother with references...
		Nat objId = objIds->get(v, objIds->count());
		to->writeNat(objId);

		if (objId != objIds->count()) {
			// Already existing object?
			return false;
		}

		// New object. Write its actual type.
		to->writeNat(typeId(type->type) & ~typeMask);
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

	Type *ObjOStream::start(TypeDesc *type) {
		Type *r = type->type;
		if (depth->empty()) {
			// We're the root object, write a header.
			to->writeNat(typeId(type->type) & ~typeMask);
		} else {
			Cursor &at = depth->last();
			// Note: Will crash if we ever let a custom serialization call "regular" serialization (eg. containers).
			r = at.current().type;
			at.next();
		}

		// Add a cursor to 'depth' to keep track of what we're doing!
		depth->push(Cursor(type));
		return r;
	}

	void ObjOStream::end() {
		if (depth->empty())
			throw SerializationError(L"Mismatched calls to startX and end during serialization!");

		Cursor end = depth->last();
		if (end.more())
			throw SerializationError(L"Missing fields during serialization!");

		depth->pop();
	}

	void ObjOStream::writeInfo(TypeDesc *t, Byte flags) {
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
			const TypeMember &member = t->members->at(i);

			Nat id = typeId(member.type);
			to->writeNat(id & ~typeMask);
			member.name->write(to);
		}

		// End of members.
		to->writeNat(endId);
	}

}

