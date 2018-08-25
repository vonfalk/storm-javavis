#pragma once
#include "Core/Array.h"
#include "Core/Map.h"
#include "Core/CloneEnv.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	class StoredType;

	/**
	 * Description of a stored field in a serialized type.
	 */
	class StoredField {
		STORM_VALUE;
	public:
		// Create.
		STORM_CTOR StoredField(Str *name, StoredType *type, Nat offset);

		// Name of the field.
		Str *name;

		// Type of the field. TODO: Replace with a function that generates a 'StoredType' object?
		// Otherwise, it will be tricky to generate instances of 'StoredType' using initializers only.
		StoredType *type;

		// Offset of the field inside the type.
		Nat offset;

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);
	};


	/**
	 * Description of a serialized type.
	 *
	 * This description is similar to core.lang.Type, but is different since it focuses on the
	 * serialized data. Instances of this type are associated to a core.lang.Type, but do not have
	 * to correspond exactly to the contents of the type. This is to support cases when the
	 * underlying type has been modified since it was serialized.
	 */
	class StoredType : public Object {
		STORM_CLASS;
	public:
		// Create and associate with a type.
		STORM_CTOR StoredType(Type *t);

		// Copy.
		StoredType(const StoredType &o);

		// Deep copy.
		void STORM_FN deepCopy(CloneEnv *env);

		// Associated type.
		Type *type;

		// Number of fields.
		Nat STORM_FN count() const { return fields->count(); }

		// Get a field.
		StoredField STORM_FN at(Nat i) const { return fields->at(i); }

		// Add a field.
		void STORM_FN push(StoredField f) { fields->push(f); }

	private:
		// Type information.
		Array<StoredField> *fields;
	};


	/**
	 * Input stream for objects.
	 *
	 * Note that the serialization mechanism may store metadata required for multiple objects only
	 * once in the stream, which means that objects written using a single instance of an OObjStream
	 * has to be read using a single instance of an IObjStream.
	 */
	class IObjStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR IObjStream(IStream *src);

	private:
		// Source stream.
		IStream *src;
	};


	/**
	 * Output stream for objects.
	 */
	class OObjStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR OObjStream(OStream *dst);

		// Write an object to the stream. 'value' is a pointer to an instance of the type described
		// by 'type', similar to how types are handled with the Handle type.
		void CODECALL write(StoredType *type, const void *value);

	private:
		// Destination stream.
		OStream *dst;

		// Directory of all types that have been stored so far.
		Map<Type *, StoredType *> *stored;

		// Directory of allocated type id:s.
		Map<Type *, Nat> *ids;

		// Next available type id.
		Nat nextId;
	};


	/**
	 * Reserved type id:s. These denote the primitive types known natively by the serialization system.
	 */
	enum StoredId {
		byteId = 1,
		intId = 2,
		natId = 3,
		longId = 4,
		wordId = 5,
		floatId = 6,
		doubleId = 7,
		stringId = 8,

		// First ID usable by custom types.
		firstCustomId = 0x20
	};

}
