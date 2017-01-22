#pragma once
#include "Named.h"
#include "Thread.h"
#include "NamedThread.h"
#include "Scope.h"
#include "Package.h"
#include "TypeChain.h"
#include "TypeLayout.h"
#include "VTable.h"
#include "RefHandle.h"
#include "Code/Value.h"
#include "Shared/TypeFlags.h"
#include "AsRef.h"
#include "OverloadPart.h"

namespace storm {
	STORM_PKG(core.lang);

	class TypeVar;

	/**
	 * Represents a type. This class contains information about
	 * all type members, fields and so on.
	 *
	 * TODO: Make it possible to create a Type from Storm!
	 *
	 * The special name "__ctor" is reserved for the constructor.
	 * A constructor is special in the way that it does not take a parameter
	 * to the object itself as its first parameter. Instead it takes a parameter
	 * to the current Type-object instead. This is enforced by the 'add' method.
	 */
	class Type : public NameSet {
		STORM_CLASS;
	public:
		// Create a type that exists in Storm.
		Type(const String &name, TypeFlags flags, const vector<Value> &params = vector<Value>(), Size size = Size());

		// Create a type declared in C++.
		Type(const String &name, TypeFlags flags, Size size, void *cppVTable);

		// Dtor.
		~Type();

		// Create the first type instance, as an instance of itself.
		static Type *createType(Engine &engine, const String &name, TypeFlags flags);

		static const String CTOR;
		static const String DTOR;

		// Owning engine. Currently this has to be the first member in Type. Otherwise modify createType.
		// TODO: Rename so that it does not collide with engine() in Object (confusing!).
		Engine &engine;

		// Type flags.
		const TypeFlags typeFlags;

		// Reference to this Type that will be kept updated through any renames.
		code::RefSource typeRef;

		// Get the size of this type. NOTE: This is safe to call from any thread (the only one that is safe).
		Size CODECALL size(); // const;

		// Get a handle to this type. It will be updated as long as this type lives.
		const Handle &handle();

		// Set parent type. The parent type has to have the same type parameters as this one.
		void setSuper(Par<Type> super);

		// Associate this type with a thread. Threads are inherited by all child objects, and
		// it is only allowed to set the thread on a root object type. (threads does not make sense
		// with values).
		void setThread(Par<NamedThread> thread);

		// Get super type.
		Type *super() const;

		// Get the thread we want to run on.
		RunOn runOn() const;

		// Any super type the given type?
		bool isA(const Type *super) const;

		// Distance in the inheritance chain? Negative if !isA(from)
		int distanceFrom(Type *from) const;

		// Add new members.
		using NameSet::add;
		virtual void STORM_FN add(Par<Named> m);

		// Clear contents. Mainly used for clean exits.
		virtual void clear();

		// What kind of built-in type is this? Returns 'user' if it is not a built-in type.
		virtual BasicTypeInfo::Kind builtInType() const { return BasicTypeInfo::user; }

		// Get a pointer/reference to the destructor (if any).
		Function *destructor();

		// Get a pointer/reference to the copy constructor (if any).
		Function *copyCtor();

		// Pointer to the current copy-constructor of this type. Updated to match any changes. Null if none.
		const void *copyCtorFn();

		// Get a pointer/reference to the default constructor (if any).
		Function *defaultCtor();

		// Get a pointer/reference to the assignment operator (if any).
		Function *assignFn();

		// Deep copy function.
		Function *deepCopyFn();

		// Equals function. Note: may be T.equals(T) or T.equals(Object), the latter may be removed in the future.
		Function *equalsFn();

		// Hash function.
		Function *hashFn();

		// Default ctor function for a handle. Returns a void (*)(void *to), where to points to the
		// memory to be filled out. In the case of pointers, this is a pointer to the pointer to be
		// filled. Returns 'null' if defaultCtor() returns null.
		code::RefSource *handleDefaultCtor();

		// ToS function. NOTE: This may return different values as the declaration changes (eg. toS
		// is moved from the surrounding scope to this scope). This is not properly supported yet,
		// and automatic updates of this will not be done.
		Function *findToSFn();

		// Get the offset to a member. TODO: Maybe replace this one with RefSources in TypeVar?
		Offset offset(const TypeVar *var) const;

		// Get all current member variables (not from super-classes).
		vector<Auto<TypeVar> > variables() const;

		// Type chain.
		TypeChain chain;

		// VTable for this type. Not too useful if we're a value type, but maintained anyway for simplicity.
		VTable vtable;

		// Lazy loading callback from NameSet. Note: remember to call this function, at least when the load
		// succeeded!
		virtual Bool STORM_FN loadAll();

		// Find stuff.
		virtual MAYBE(Named) *STORM_FN find(Par<SimplePart> part);

	protected:
		virtual void output(wostream &to) const;

	private:
		// Validate parameters to added members.
		void validate(Named *m);

		// Our parent type's size.
		Size superSize() const;

		// Our size (including base classes). If it is zero, we need to re-compute it!
		Size mySize;

		// Variable layout.
		TypeLayout layout;

		// Type handle.
		RefHandle *typeHandle;

		// Cache of commonly accessed functions (borrowed ptrs). For some reason, these can not be Auto<>.
		Function *copyCtorCache;
		Function *defaultCtorCache;
		Function *dtorCache;

		// Which thread should we be running on?
		Auto<NamedThread> thread;

		// Convert the hash() function and the equals() function to take references if needed.
		AsRef *hashAsRef, *equalsAsRef;

		// Default constructor that writes its output to the target of a pointer. Intended for use
		// with Handle things.
		code::RefSource *handleDefaultCtorFn;

		// Update the handle.
		void updateHandle(bool force);

		// Update the output part of the handle.
		void updateHandleOutput();

		// Ensure that any lazy-loaded parts are loaded.
		void ensureLoaded();

		// Init (shared parts of constructors). Note that this takes unmasked flags!
		// The reason for this is that we want to know about 'typeManualSuper', which is never
		// present in typeFlags.
		void init(TypeFlags typeFlags);

		// Update the need for virtual calls for all members.
		void updateVirtual();

		// Update the need for firtual calls for this functions and any parent functions found.
		void updateVirtual(Named *named);

		// Update the virtual state of a single function.
		void updateVirtualHere(Function *fn, OverloadPart *part);

		// Check if 'x' needs to have a virtual dispatch.
		bool needsVirtual(Function *fn, OverloadPart *part);

		// Enable/disable vtable lookup for the given function.
		void enableLookup(Function *fn, OverloadPart *part);
		void disableLookup(Function *fn);

		// Find a possible overload to a function here. Does not attempt to lazy-load.
		Function *overloadTo(Function *fn, OverloadPart *part);

		// Insert all known overloads to 'fn' into the respective types vtables.
		void insertOverloads(Function *fn, OverloadPart *base);
	};

	// STORM versions of the 'getDefaultXxx'
	MAYBE(Function) *STORM_FN emptyCtor(Par<Type> t) ON(Compiler);
	MAYBE(Function) *STORM_FN copyCtor(Par<Type> t) ON(Compiler);
	MAYBE(Function) *STORM_FN dtor(Par<Type> t) ON(Compiler);
	MAYBE(Function) *STORM_FN assignFn(Par<Type> t) ON(Compiler);
	MAYBE(Function) *STORM_FN deepCopyFn(Par<Type> t) ON(Compiler);
	MAYBE(Function) *STORM_FN equalsFn(Par<Type> t) ON(Compiler);
	MAYBE(Function) *STORM_FN hashFn(Par<Type> t) ON(Compiler);
	MAYBE(Function) *STORM_FN findToSFn(Par<Type> t) ON(Compiler);

	wrap::Size STORM_FN size(Par<Type> t);
}
