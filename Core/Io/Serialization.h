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
		STORM_CTOR StoredField(Str *name, Type *type, Nat offset);

		// Name of the field.
		Str *name;

		// Type of the field.
		Type *type;

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
	class ObjIStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ObjIStream(IStream *src);

	private:
		// Source stream.
		IStream *src;
	};


	/**
	 * Output stream for objects.
	 */
	class ObjOStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ObjOStream(OStream *to);

		// Destination stream. Used to implement custom serialization.
		OStream *to;

		// Write an object to the stream. 'value' is a pointer to an instance of the type described
		// by 'type', similar to how types are handled with the Handle type.
		void CODECALL write(StoredType *type, const void *value);

		// Write various primitive types to the stream (using 'write' above).
		void STORM_FN writeByte(Byte v);
		void STORM_FN writeInt(Int v);
		void STORM_FN writeNat(Nat v);
		void STORM_FN writeLong(Long v);
		void STORM_FN writeWord(Word v);
		void STORM_FN writeFloat(Float v);
		void STORM_FN writeDouble(Double v);

	private:

		// Directory of all types that have been stored so far. Note: We're using TObject, since
		// Type is not accessable outside the compiler.
		Map<TObject *, StoredType *> *stored;

		// Directory of allocated type id:s.
		Map<TObject *, Nat> *ids;

		// Next available type id.
		Nat nextId;

		// Serialize a type description. Makes sure that all subtypes referred to inside 'type' are assigned id:s.
		void writeTypeDesc(StoredType *type);

		// Serialize a field.
		void writeField(StoredField field);

		// Find the type id for 't'. Generate a new one if none exists.
		Nat findId(Type *t);
	};


	/**
	 * Reserved type id:s. These denote the primitive types known natively by the serialization system.
	 */
	enum StoredId {
		// Built in types.
		byteId = 0x01,
		intId = 0x02,
		natId = 0x03,
		longId = 0x04,
		wordId = 0x05,
		floatId = 0x06,
		doubleId = 0x07,
		strId = 0x08,

		// Containers.
		arrayId = 0x10,
		mapId = 0x11,
		setId = 0x12,
		pqId = 0x13,

		// First ID usable by custom types.
		firstCustomId = 0x20
	};

}
