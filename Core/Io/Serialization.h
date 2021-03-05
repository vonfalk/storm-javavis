#pragma once
#include "Utils/Exception.h"
#include "Utils/Bitmask.h"
#include "Core/Array.h"
#include "Core/Map.h"
#include "Core/CloneEnv.h"
#include "Core/Fn.h"
#include "Core/Variant.h"
#include "Core/Unknown.h"
#include "Stream.h"

namespace storm {
	STORM_PKG(core.io);

	namespace typeInfo {
		/**
		 * Information about a serialized type
		 */
		enum TypeInfo {
			// Nothing in particular.
			none = 0x00,

			// Is this a class-type? If not set, it is a value type.
			classType = 0x01,

			// Is this a tuple-type?
			tuple = 0x02,

			// Is this a maybe-type?
			maybe = 0x04,

			// Completely custom type?
			custom = 0x08,
		};

		BITMASK_OPERATORS(TypeInfo);
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
	 * Description of a class that is serialized. Instances of the subclass, SerializedType,
	 * describe classes consisting of a fixed set of data members (that are serialized
	 * automatically), while instances of this class may use custom serialization mechanisms.
	 *
	 * All serializable classes need to provide their superclass (if any), and a function pointer to
	 * a constructor that deserializes the object.
	 */
	class SerializedType : public Object {
		STORM_CLASS;
	public:
		// Create a description of an object of the type 't'.
		STORM_CTOR SerializedType(Type *t, FnBase *ctor);

		// Create a description of an object of the type 't', whith the super type 'p'.
		STORM_CTOR SerializedType(Type *t, FnBase *ctor, SerializedType *super);

		// The type.
		Type *type;

		// Function pointer to the constructor of the class.
		FnBase *readCtor;

		// Super type.
		inline MAYBE(SerializedType *) STORM_FN super() const { return mySuper; }

		// Get the TypeInfo bitmask for this type.
		typeInfo::TypeInfo STORM_FN info() const;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		/**
		 * Cursor. The first element will be the super type, if present.
		 */
		class Cursor {
			STORM_VALUE;
		public:
			// Create. Empty.
			STORM_CTOR Cursor();

			// Create, referring to a type.
			STORM_CTOR Cursor(SerializedType *type);

			// Referring to the parent class?
			inline Bool STORM_FN isParent() const { return pos == 0; }

			// Referring to any element?
			inline Bool STORM_FN any() const { return type && pos < type->types->count() + 1; }

			// At the end? Closely related to 'any', but returns 'true' if a complete tuple has been writen as well.
			inline Bool STORM_FN atEnd() const { return !type || pos == type->typesRepeat + 1; }

			// Current element?
			inline Type *STORM_FN current() const { return (pos == 0) ? type->mySuper->type : type->typeAt(pos - 1); }

			// Advance.
			void STORM_FN next();

		private:
			// The type.
			MAYBE(SerializedType *) type;

			// Position.
			Nat pos;
		};

	protected:
		// Overridden in subclasses to provide the base info.
		virtual typeInfo::TypeInfo STORM_FN baseInfo() const;

		// Access the type storage.
		inline void typeAdd(Type *t) { types->push((TObject *)t); }

		// Get a type.
		inline Type *typeAt(Nat id) const { return (Type *)types->at(id); }

		// Number of types.
		inline Nat typeCount() const { return types->count(); }

		// Set the repeat.
		inline void typeRepeat(Nat r) { typesRepeat = r; }

	private:
		// The super type.
		MAYBE(SerializedType *) mySuper;

		// All types in this class, so that the Cursor can iterate over them easily.
		Array<TObject *> *types;

		// Are the types above repeating? From which index in that case? If 'typesRepeat >=
		// types->count()', then no repeat is in effect.
		Nat typesRepeat;

		// Common initialization.
		void init();
	};


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
	class SerializedStdType : public SerializedType {
		STORM_CLASS;
	public:
		// Create a description of an object of the type 't'.
		STORM_CTOR SerializedStdType(Type *t, FnBase *ctor);

		// Create a description of an object of the type 't', with the super type 'p'.
		STORM_CTOR SerializedStdType(Type *t, FnBase *ctor, SerializedType *super);

		// Add a member.
		void STORM_FN add(Str *name, Type *type);

		// Number of members.
		inline Nat STORM_FN count() const { return names->count(); }

		// Get a member.
		SerializedMember at(Nat i) const;

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	protected:
		typeInfo::TypeInfo STORM_FN baseInfo() const;

	private:
		// Names of the members.
		Array<Str *> *names;
	};


	/**
	 * Description of a sequence of tuples. Used to serialize containers using a format
	 * understandable by readers unaware of the underlying types.
	 *
	 * The standard serialization mechanisms do not use this serialization automatically; custom
	 * serialization is required. However, in practice this will not be required as the containers
	 * implement the appropriate custom serialization automatically.
	 *
	 * An array is just a sequence of 1-tuples, while a map is a sequence of 2-tuples, for example.
	 */
	class SerializedTuples : public SerializedType {
		STORM_CLASS;
	public:
		// Create a type description.
		STORM_CTOR SerializedTuples(Type *t, FnBase *ctor);
		STORM_CTOR SerializedTuples(Type *t, FnBase *ctor, SerializedType *super);

		// Add a type.
		void STORM_FN add(Type *t) { typeAdd(t); }

		// Count.
		Nat STORM_FN count() const { return typeCount() - 1; }

		// Element.
		Type *STORM_FN at(Nat i) const { return typeAt(i + 1); }

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	protected:
		typeInfo::TypeInfo STORM_FN baseInfo() const;
	};


	/**
	 * Description of a serialized maybe-type. Fairly similar to "SerializedTuples", but expects a
	 * boolean to be written first rather than an entire integer.
	 */
	class SerializedMaybe : public SerializedType {
		STORM_CLASS;
	public:
		// Create a type description.
		STORM_CTOR SerializedMaybe(Type *t, FnBase *ctor, Type *contained);

		// Get the contained type.
		inline Type *STORM_FN contained() const { return typeAt(1); }

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

	protected:
		typeInfo::TypeInfo STORM_FN baseInfo() const;
	};


	/**
	 * Error during serialization.
	 */
	class EXCEPTION_EXPORT SerializationError : public Exception {
		STORM_EXCEPTION;
	public:
		SerializationError(const wchar *msg) {
			w = new (this) Str(msg);
			saveTrace();
		}
		STORM_CTOR SerializationError(Str *msg) {
			w = msg;
			saveTrace();
		}

		virtual void STORM_FN message(StrBuf *to) const {
			*to << w;
		}
	private:
		Str *w;
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

		// Deserialize a value of the given type. The type is stored in memory previously allocated
		// and passed in as 'out'. This memory will be initialized by this call.
		void STORM_FN readValue(Type *type, PTR_GC out);

		// Deserialize an object of the given type. The result is always an instance of the type
		// described by 'type', even if this is not possible to express in the type system.
		Object *STORM_FN readClass(Type *type);

		// Read a primitive value of some sort.
		void readPrimitiveValue(StoredId id, PTR_GC out);
		Object *readPrimitiveObject(StoredId id);

		// Indicate the end of an object.
		void STORM_FN end();

	private:
		// Description of a member.
		//
		// Note that since some members are read in multiple steps, the same name is possibly
		// repeated inside a single Desc instance.
		class Member {
			STORM_VALUE;
		public:
			// Create.
			Member(Str *name, Nat type);

			// Create a copy with a different 'read' member.
			Member(const Member &o, Int read);

			// Name of the member.
			MAYBE(Str *) name;

			// Type of the member.
			Nat type;

			// How to read the member:
			// =0: just read the member as usual.
			// <0: read this member and push it to temporary storage. Do not return it immediately.
			// >0: read the member from an object previously saved in temporary storage, not from the stream.
			// This allows reading members in a different order compared to what is expected
			// by the constructor of the object we're constructing. It does, however, incur
			// additional memory allocation in cases where we need to reorder members. This is
			// fine since we expect that case to be rare.
			Int read;
		};

		// Description of a type. Contains a pointer to an actual type once we know that.
		class Desc : public Object {
			STORM_CLASS;
		public:
			// Create.
			Desc(Byte flags, Nat parent, Str *name);

			// Create as a wrapper for a built-in type.
			Desc(Byte flags, Type *type, FnBase *ctor);

			// Data. Flags (in the highest 8 bits) and number of temporary entries required.
			Nat data;

			// Flags.
			Byte flags() const { return (data >> 24) & 0xFF; }

			// Is this a value type?
			Bool isValue() const { return (flags() & typeInfo::classType) == typeInfo::none; }

			// Is this a tuple?
			Bool isTuple() const { return (flags() & typeInfo::tuple) != 0; }

			// Is this a maybe-type?
			Bool isMaybe() const { return (flags() & typeInfo::maybe) != 0; }

			// Does this type allow early-out?
			Bool isEarlyOut() const { return (flags() & (typeInfo::maybe | typeInfo::tuple)) != 0; }

			// Entries required in the temporary storage.
			Nat storage() const { return data & 0x00FFFFFF; }

			// Set entries in temporary storage.
			void storage(Nat v) { data = (Nat(flags()) << 24) | (v & 0x00FFFFFF); }

			// ID of the parent class.
			Nat parent;

			// Members. If 'null', this type has custom serialization.
			Array<Member> *members;

			// What we know about the serialization.
			SerializedType *info;

			// Find a member by name. Returns an id >= members->count() on failure.
			Nat findMember(Str *name) const;
		};

		// Cursor into the Desc object. Only worries about members in contrast to the cursor in
		// SerializedType.
		class Cursor {
			STORM_VALUE;
		public:
			Cursor();
			Cursor(Desc *desc);

			// Does this cursor refer to a description of a custom type?
			inline Bool customDesc() const { return desc->members == null; }

			// Any element?
			inline Bool any() const { return desc && desc->members && pos < desc->members->count(); }

			// At the end of serialization? Allows repetition in case of tuples.
			inline Bool atEnd() const {
				return !desc || !desc->members || pos >= desc->members->count() || (pos == 1 && desc->isEarlyOut());
			}

			// Current element?
			const Member &current() const;

			// Advance.
			void next();

			// Access an element in the temporary storage.
			Variant &temporary(Nat n) { return tmp->v[n]; }

			// Add a new temporary object.
			void pushTemporary(const Variant &v);

		private:
			Desc *desc;
			GcArray<Variant> *tmp;
			Nat pos;
		};

		// Keep track of how the serialization is progressing. Used as a stack.
		Array<Cursor> *depth;

		// Directory of previously deserialized objects.
		Map<Nat, Object *> *objIds;

		// Directory of known type ids.
		Map<Nat, Desc *> *typeIds;

		// Information from 'start'.
		struct Info {
			// Type we're expecting.
			Nat expectedType;

			// Set if a previously existing value should be returned.
			Variant result;
		};

		// Start deserialization of an object. As a part of the process, figures out which object to
		// read and returns its id. Will also make sure to read any objects that are supposed to be
		// stored in temporary storage if necessary.
		Info start();

		// Get the description for an object id, reading it if necessary.
		Desc *findInfo(Nat id);

		// Validate a type description for members against our view of the corresponding type.
		void validateMembers(Desc *desc);

		// Validate a type description for tuples against our view of the corresponding type.
		void validateTuple(Desc *desc);

		// Validate a maybe-description.
		void validateMaybe(Desc *desc);

		// Read an object into a variant.
		Variant readClass(Nat type);

		// Read a value (internal version, does not call 'start').
		void readValueI(Desc *type, void *out);

		// Read a class (internal version, does not call 'start').
		Object *readClassI(Desc *type, Type *t);

		// Clear object ids.
		void clearObjects();
	};


	/**
	 * Output stream for objects.
	 *
	 * Generic serialization is implemented as follows:
	 * 1: call 'startValue', 'startClass' depending on what is appropriate.
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
		Bool STORM_FN startClass(SerializedType *type, Object *v);

		// Indicate the start of serialization of a primitive type.
		void STORM_FN startPrimitive(StoredId id);

		// Indicate the end of an object serialization. Not called if 'startXxx' returned false.
		void STORM_FN end();

		// Flush the stream.
		void STORM_FN flush() { to->flush(); }

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
		void writeInfo(SerializedType *desc);
	};

	// Demangle a name.
	Str *STORM_FN demangleName(Str *name);
}
