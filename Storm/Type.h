#pragma once
#include "Named.h"
#include "Thread.h"
#include "NamedThread.h"
#include "Scope.h"
#include "Package.h"
#include "TypeChain.h"
#include "TypeLayout.h"
#include "VTable.h"
#include "Lib/Handle.h"
#include "Code/Value.h"

namespace storm {
	STORM_PKG(core.lang);

	class TypeVar;

	/**
	 * Define different properties for a type.
	 */
	enum TypeFlags {
		// Regular type.
		typeClass = 0x01,

		// Is it a value type (does not inherit from Object).
		typeValue = 0x02,

		// Final (not possible to override)?
		typeFinal = 0x10,

		// Do not setup inheritance automatically (cleared in the constructor). If set,
		// you are required to manually set classes to inherit from Object.
		typeManualSuper = 0x80,
	};

	BITMASK_OPERATORS(TypeFlags);

	/**
	 * Represents a type. This class contains information about
	 * all type members, fields and so on.
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
		Type(const String &name, TypeFlags flags, const vector<Value> &params = vector<Value>());

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
		const TypeFlags flags;

		// Reference to this Type that will be kept updated through any renames.
		code::RefSource typeRef;

		// Get the size of this type.
		Size size(); // const;

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
		bool isA(Type *super) const;

		// Add new members.
		virtual void STORM_FN add(Par<Named> m);

		// Clear contents. Mainly used for clean exits.
		void clear();

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

		// Get the offset to a member. TODO: Maybe replace this one with RefSources in TypeVar?
		Offset offset(const TypeVar *var) const;

		// Get all current member variables (not from super-classes).
		vector<Auto<TypeVar> > variables() const;

		// Type chain.
		TypeChain chain;

		// VTable for this type. Not too useful if we're a value type, but maintained anyway for simplicity.
		VTable vtable;

	protected:
		virtual void output(wostream &to) const;

		// Find stuff.
		virtual Named *findHere(const String &name, const vector<Value> &params);

		// Called before the first time any information is wanted.
		// NOTE: Inheritance information is assumed to be set up in a non-lazy way!
		virtual void lazyLoad();

		// Call to inhibit lazy-loading until later.
		void allowLazyLoad(bool v);

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
		RefHandle typeHandle;

		// Which thread should we be running on?
		Auto<NamedThread> thread;

		// Loaded the lazy parts?
		bool lazyLoaded;

		// Loading the lazy parts?
		bool lazyLoading;

		// Update the handle.
		void updateHandle(bool force);

		// Ensure that any lazy-loaded parts are loaded.
		void ensureLoaded();

		// Init (shared parts of constructors).
		void init(TypeFlags flags);

		// Update the need for virtual calls for all members.
		void updateVirtual();

		// Update the need for firtual calls for this functions and any parent functions found.
		void updateVirtual(Named *named);

		// Update the virtual state of a single function.
		void updateVirtualHere(Function *fn);

		// Check if 'x' needs to have a virtual dispatch.
		bool needsVirtual(Function *fn);

		// Enable/disable vtable lookup for the given function.
		void enableLookup(Function *fn);
		void disableLookup(Function *fn);

		// Find a possible overload to a function. Does not attempt to lazy-load.
		Function *overloadTo(Function *base);

		// Insert all known overloads to 'fn' into the respective types vtables.
		void insertOverloads(Function *base);
	};

}
