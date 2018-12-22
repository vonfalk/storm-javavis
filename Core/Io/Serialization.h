#pragma once
#include "Utils/Exception.h"
#include "Utils/Bitmask.h"
#include "Core/Array.h"
#include "Core/Map.h"
#include "Core/CloneEnv.h"
#include "Core/Fn.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	/**
	 * A single member inside an object description.
	 */
	class SerializedMember {
		STORM_VALUE;
	public:
		STORM_CTOR SerializedMember(Str *name, Type *type);

		Str *name;
		Type *type;
	};


	/**
	 * Description of the members of a class that are being serialized. Used with the standard
	 * serialization mechanisms in ObjIStream and ObjOStream.
	 */
	class SerializedType : public Object {
		STORM_CLASS;
	public:
		// Create a description of an object of the type 't'.
		STORM_CTOR SerializedType(Type *t, FnBase *ctor);

		// Create a description of an object of the type 't', with the parent 'p'.
		STORM_CTOR SerializedType(Type *t, FnBase *ctor, SerializedType *parent);

		// The type.
		Type *type;

		// Function pointer to the constructor of the class.
		FnBase *readCtor;

		// Parent type.
		MAYBE(SerializedType *) parent;

		// All fields in here.
		Array<SerializedMember> *members;

		// Add a member.
		void STORM_FN add(Str *name, Type *type);

		/**
		 * Cursor into a SerializedType.
		 *
		 * Note: Treats the parent class (if any) as the first member.
		 */
		class Cursor {
			STORM_VALUE;
		public:
			STORM_CTOR Cursor();
			STORM_CTOR Cursor(SerializedType *type);

			// Is this cursor referring to a parent class?
			inline Bool STORM_FN isParent() const { return pos == 0; }

			// More elements?
			inline Bool STORM_FN more() const { return type && pos <= type->members->count(); }

			// Current element?
			Type *STORM_FN current() const;

			// Advance.
			inline void STORM_FN next() { if (more()) pos++; }

		private:
			// Note: Type may be null.
			SerializedType *type;
			Nat pos;
		};
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

		// Deserialize an object of the given type. The result is always an instance of the type
		// described by 'type', even if this is not possible to express in the type system.
		Object *STORM_FN readObject(Type *type);


		// Start deserialization of an object. Returns either an instance of the object, or null to
		// indicate that deserialization shall be performed here by calling the constructor.
		// MAYBE(Object *) STORM_FN startObject(SerializedType *type);

		// Indicate the start of a custom type.
		void STORM_FN startCustom(StoredId id);

		// Indicate the end of an object.
		void STORM_FN end();

	private:
		// Description of a type. Contains a pointer to an actual type once we know that.
		class Desc : public Object {
			STORM_CLASS;
		public:
			// Create.
			Desc(Byte flags, Nat parent, Str *name);

			// Flags.
			Byte flags;

			// ID of the parent class.
			Nat parent;

			// Member names and types.
			Array<Str *> *memberNames;
			Array<Nat> *memberTypes;

			// What we know about the serialization.
			SerializedType *info;
		};

		// Cursor into the Desc object. Only worries about members in contrast to the cursor in
		// SerializedType.
		class Cursor {
			STORM_VALUE;
		public:
			Cursor();
			Cursor(Desc *desc);

			// More elements?
			inline Bool more() const { return desc && pos < desc->memberTypes->count(); }

			// Current element?
			Nat current() const;

			// Advance.
			inline void next() { if (more()) pos++; }

		private:
			Desc *desc;
			Nat pos;
		};

		// Keep track of how the serialization is progressing. Used as a stack.
		Array<Cursor> *depth;

		// Directory of previously deserialized objects.
		Map<Nat, Object *> *objIds;

		// Directory of known type ids.
		Map<Nat, Desc *> *typeIds;

		// Figure out the type we're supposed to read at the moment. Assumes we start reading that
		// type afterwards, as internal state is updated so that the next call to 'expectedType'
		// returns something different.
		Nat expectedType();


		// Start a serialization. Returns the object id we're expecting to read, after possibly
		// reading it.
		Desc *start(SerializedType *type);

		// Get the description for an object id, reading it if necessary.
		Desc *findInfo(Nat id);

		// Clear object ids.
		void clearObjects();
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
		Bool STORM_FN startValue(SerializedType *type);
		Bool STORM_FN startObject(SerializedType *type, Object *v);

		// Indicate the start of serialization of a custom type.
		void STORM_FN startCustom(Type *type);
		void STORM_FN startCustom(StoredId id);

		// Indicate the end of an object serialization. Not called if 'startXxx' returned false.
		void STORM_FN end();

	private:
		// Keep track of how the serialization is progressing. Used as a stack.
		Array<SerializedType::Cursor> *depth;

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
		// If we're expecting to serialize a parent type to a previously serialized type, "null" is returned.
		MAYBE(Type *) start(SerializedType *type);

		// Write the type info provided, if needed.
		void writeInfo(SerializedType *desc, Byte flags);
	};

	// Demangle a name.
	Str *STORM_FN demangleName(Str *name);
}
