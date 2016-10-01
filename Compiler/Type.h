#pragma once
#include "NameSet.h"
#include "TypeChain.h"
#include "Gc.h"
#include "RunOn.h"
#include "Core/TypeFlags.h"

namespace storm {
	STORM_PKG(core.lang);

	// Description of a type in C++. Found in CppTypes.h
	struct CppType;
	class NamedThread;

	/**
	 * Description of a type.
	 */
	class Type : public NameSet {
		STORM_CLASS;

		// Let allocation access gcType.
		friend void *runtime::allocObject(size_t s, Type *t);

	public:
		// Create a type declared in Storm.
		STORM_CTOR Type(Str *name, TypeFlags flags);
		STORM_CTOR Type(Str *name, Array<Value> *params, TypeFlags flags);

		// Create a type declared in C++.
		Type(Str *name, TypeFlags flags, Size size, GcType *gcType);

		// Destroy our resources.
		~Type();

		// Owning engine. Must be the first data member of this class!
		Engine &engine;

		// Create the type for Type (as this is special).
		static Type *createType(Engine &e, const CppType *type);

		// Set this type onto another object. This can *not* be used to trick the system into
		// resizing the object. This is only used once during startup.
		void setType(Object *object) const;

		// Set the super-class for this type.
		void STORM_FN setSuper(Type *to);

		// Get the super-class for this type.
		inline MAYBE(Type *) STORM_FN super() { return chain->super(); }

		// Set the thread for this type. This will force the super-type to be TObject.
		void STORM_FN setThread(NamedThread *t);

		// Get where we want to run.
		RunOn STORM_FN runOn();

		// Get a handle for this type.
		const Handle &handle();

		// Late initialization.
		virtual void lateInit();

		// Get this class's size. (NOTE: This function is safe to call from any thread).
		Size STORM_FN size();

		// Inheritance chain and membership lookup. TODO: Make private?
		TypeChain *chain;

		// Helpers for the chain.
		inline Bool STORM_FN isA(const Type *o) const { return chain->isA(o); }

		// To string.
		virtual void STORM_FN toS(StrBuf *to) const;

		// Flags for this type.
		const TypeFlags typeFlags;

		// What kind of type is this type?
		virtual BasicTypeInfo::Kind builtInType() const;

	private:
		// Special constructor for creating the first type.
		Type(Engine &e, TypeFlags flags, Size size, GcType *gcType);

		// The description of the type we maintain for the GC. If we're a value type,
		// this will have 'kind' set to 'tArray'.
		// Note: the type member of the GcType this is pointing to must never be changed after this
		// member is changed. Otherwise we confuse the GC.
		GcType *gcType;

		// Handle (lazily created).
		const Handle *tHandle;

		// Thread we should be running on if we indirectly inherit from TObject.
		NamedThread *useThread;

		// Our size (including base classes). If zero, we need to re-compute it.
		Size mySize;

		// Special case for the first Type.
		static void *operator new(size_t size, Engine &e, GcType *type);
		static void operator delete(void *mem, Engine &e, GcType *type);

		// Common initialization.
		void init();

		// Is this a value type?
		inline bool value() const { return (typeFlags & typeValue) == typeValue; }

		// Generate a handle for this type.
		const Handle *buildHandle();

		// Notify that the thread changed.
		void notifyThread(NamedThread *t);

		// Compute the size of our super-class.
		Size superSize();
	};


}
