#pragma once
#include "Utils/Exception.h"
#include "Utils/Bitmask.h"
#include "Core/Array.h"
#include "Core/Map.h"
#include "Core/Fn.h"
#include "Core/CloneEnv.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	class TypeSerialization;


	namespace typeInfo {
		/**
		 * Information about a serialized type
		 */
		enum TypeInfo {
			// Value type.
			valueType = 0x00,

			// Class type.
			classType = 0x01,
		};
	}


	/**
	 * Description of an automatically serialized member.
	 */
	class MemberInfo {
		STORM_VALUE;
	public:
		STORM_CTOR MemberInfo(Str *name, Nat offset, Fn<TypeSerialization *> *type);

		// Name of the member.
		Str *name;

		// Offset of the member inside the class.
		Nat offset;

		// Pointer to a function that generates information about this member.
		Fn<TypeSerialization *> *type;
	};


	/**
	 * Description of how to serialize a class. There are two ways of doing this:
	 *
	 * 1. The description contains a description of the members of the class. Serialization will be
	 * performed automatically according to this information.
	 *
	 * 2. The description contains a serialization and deserialization function which perform
	 * serialization and deserialization respectively.
	 *
	 * Instances of these objects are immutable, so that we can avoid copies even though we
	 * serialize from multiple threads.
	 */
	class TypeSerialization : public Object {
		STORM_CLASS;
	public:
		// Create for automatic serialization.
		STORM_CTOR TypeSerialization(Type *type, Array<MemberInfo> *members);

		// Create for automatic serialization, with a parent class.
		STORM_CTOR TypeSerialization(Type *type, Array<MemberInfo> *members, Fn<TypeSerialization *> *parent);

		// Create for manual serialization.
		STORM_CTOR TypeSerialization(Type *type, FnBase *write, FnBase *read);

		// Is this an automatic serialization?
		Bool automatic() const { return automaticMode; }

		// Automatic serialization properties.
		Array<MemberInfo> *members() const;
		MAYBE(Fn<TypeSerialization *> *) parent() const;

		// Manual serialization properties.
		FnBase *write() const;
		FnBase *read() const;

	private:
		// The type we're representing.
		Type *type;

		// Two pointers. Either, only the first one is present, which means that we're in case
		// #1. Otherwise, both are filled and we're in case #2, and a = write, b = read.
		UNKNOWN(PTR_GC) void *a;
		UNKNOWN(PTR_GC) void *b;

		// Automatic mode?
		Bool automaticMode;

		// TODO: We need to know if we're a class or not.
	};




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
	};


	/**
	 * Reserved type id:s. These denote the primitive types known natively by the serialization system.
	 */
	enum StoredId {
		// Indicates the end of the members table.
		endId = 0x00,

		// Built in types.
		boolId = 0x01,
		byteId = 0x02,
		intId = 0x03,
		natId = 0x04,
		longId = 0x05,
		wordId = 0x06,
		floatId = 0x07,
		doubleId = 0x08,
		strId = 0x09,

		// Containers.
		arrayId = 0x10,
		mapId = 0x11,
		setId = 0x12,
		pqId = 0x13,

		// First ID usable by custom types.
		firstCustomId = 0x20
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
	 *
	 * TODO: Provide 'deepCopy' and copy ctor! Perhaps they should assert...
	 */
	class ObjOStream : public Object {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR ObjOStream(OStream *to);

		// Destination stream. Used to implement custom serialization.
		OStream *to;

		// Inidicate the start of serialization of the given object. Returns 'true' if the object
		// should be serialized, false if it has already been serialized and should be skipped
		// (including the call to 'end').
		Bool STORM_FN startValue(serialize::TypeDesc *type);
		Bool STORM_FN startObject(serialize::TypeDesc *type, Object *v);

		// Indicate the start of serialization of a custom type.
		void STORM_FN startCustom(Type *type);
		void STORM_FN startCustom(StoredId id);

		// Indicate the end of an object serialization. Not called if 'startXxx' returned false.
		void STORM_FN end();

	private:
		// Keep track of how the serialization is progressing. Used as a stack.
		Array<serialize::Cursor> *depth;

		// Directory of previously serialized objects. Note: hashes object identity rather than
		// regular equality.
		Map<Object *, Nat> *objIds;

		// Directory of allocated type id:s (actually Map<Type *, Nat>). Types that have been
		// assigned an identifier but not yet been written to the stream have their highest bit set.
		Map<TObject *, Nat> *typeIds;

		// Next available type id.
		Nat nextId;

		// Find the type id for 't'. Generate a new one if none exists.
		Nat typeId(Type *t);

		// Clear 'objIds'.
		void clearObjects();

		// Called before writing an object of type 'type'. Initializes 'depth' for the object, and
		// possibly writes the header if it was the topmost object. Returns the type we're expecting
		// to write (if we know).
		Type *start(serialize::TypeDesc *type);

		// Write the header for Type if it is needed.
		void writeHeader(Type *type, Byte flags);
	};

}
