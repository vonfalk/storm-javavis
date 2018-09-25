#pragma once
#include "Utils/Exception.h"
#include "Core/Array.h"
#include "Core/Map.h"
#include "Core/CloneEnv.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * Error during serialization.
	 */
	class EXCEPTION_EXPORT SerializationError : public Exception {
	public:
		SerializationError(const String &w) : w(w) {}
		virtual String what() const { return w; }
	private:
		String w;
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

		// Source stream.
		IStream *from;

		// Read various primitive types from the stream. Throws an exception on error.
		Byte STORM_FN readByte();
		Int STORM_FN readInt();
		Nat STORM_FN readNat();
		Long STORM_FN readLong();
		Word STORM_FN readWord();
		Float STORM_FN readFloat();
		Double STORM_FN readDouble();

		// TODO: Move to Str when we have support for static members.
		Str *STORM_FN readStr();
	};


	/**
	 * Output stream for objects.
	 *
	 * Generic serialization is implemented as follows:
	 * 1: call 'startValue', 'startObject' depending on what is appropriate.
	 * 2: if the function returns false, abort serialization of the current instance; it has already
	 *    been serialized before.
	 * 3: call 'member' once for each member that is going to be serialized.
	 * 4: call 'endMembers'.
	 * 5: serialize the members by calling their serialization function.
	 * 6: call 'end'
	 *
	 * Note: Serialization of threaded objects is not yet supported.
	 */
	class ObjOStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ObjOStream(OStream *to);

		// Destination stream. Used to implement custom serialization.
		OStream *to;

		// Write various primitive types to the stream. Used to implement custom serialization.
		void STORM_FN writeByte(Byte v);
		void STORM_FN writeInt(Int v);
		void STORM_FN writeNat(Nat v);
		void STORM_FN writeLong(Long v);
		void STORM_FN writeWord(Word v);
		void STORM_FN writeFloat(Float v);
		void STORM_FN writeDouble(Double v);

		// Indicate the start of serialization of the given object. Value types just pass the type
		// of the object. If the function returns 'false', the object was previously serialized and
		// the serialization routine shall terminate.
		void STORM_FN startValue(Type *t);
		Bool STORM_FN startObject(Object *v);

		// Indicate a new member being serialized.
		void STORM_FN member(Str *name, Type *t);

		// Indicate the end of serializing members.
		void STORM_FN endMembers();

		// Indicate the end of serialization started with a call to 'startXxx'.
		void STORM_FN end();

	private:
		// Directory of previously serialized objects. Note: hashes object identity rather than
		// regular equality.
		Map<Object *, Nat> *objIds;

		// Directory of allocated type id:s (actually Map<Type *, Nat>). Types that have been
		// assigned an identifier but not yet been written to the stream have their highest bit set.
		Map<TObject *, Nat> *typeIds;

		// Next available type id.
		Nat nextId;

		// Depth of the output.
		Nat depth;

		// Currently outputting member information for a type?
		Bool memberOutput;

		// Find the type id for 't'. Generate a new one if none exists.
		Nat typeId(Type *t);

		// Clear 'objIds'.
		void clearObjects();

		// Output any header before the current object.
		void putHeader(Type *t, Bool value);

		// Output a type description if necessary.
		void putTypeDesc(Type *t);
	};


	/**
	 * Reserved type id:s. These denote the primitive types known natively by the serialization system.
	 */
	enum StoredId {
		// Indicates the end of the members table.
		endId = 0x00,

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
