#pragma once
#include "Named.h"
#include "Scope.h"
#include "Overload.h"
#include "Package.h"
#include "TypeChain.h"
#include "TypeLayout.h"
#include "VTable.h"
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
		typeFinal = 0x04,
	};

	inline TypeFlags operator &(TypeFlags a, TypeFlags b) { return TypeFlags(nat(a) & nat(b)); }
	inline TypeFlags operator |(TypeFlags a, TypeFlags b) { return TypeFlags(nat(a) | nat(b)); }

	/**
	 * Represents a type. This class contains information about
	 * all type members, fields and so on.
	 *
	 * The special name "__ctor" is reserved for the constructor.
	 * A constructor is special in the way that it does not take a parameter
	 * to the object itself as its first parameter. Instead it takes a parameter
	 * to the current Type-object instead. This is enforced by the 'add' method.
	 */
	class Type : public Named {
		STORM_CLASS;
	public:
		// Create a type that exists in Storm.
		Type(const String &name, TypeFlags flags);

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

		// Set parent type. The parent type has to have the same type parameters as this one.
		void setSuper(Type *super);

		// Get super type.
		Type *super() const;

		// Any super type the given type?
		bool isA(Type *super) const;

		// Add new members.
		void add(NameOverload *m);

		// Lookup names.
		virtual Named *find(const Name &name);

		// Parent.
		virtual Package *parent() const { return parentPkg; }

		// Parent package, updated by Package class.
		Package *parentPkg;

		// Clear contents. Mainly used for clean exits.
		void clear();

		// Is this a type built in into the C++ compiler? These are handled differently in cdecl.
		virtual bool isBuiltIn() const { return false; }

		// Get a pointer/reference to the destructor (if any).
		virtual Function *destructor();

		// Get the offset to a member. TODO: Maybe replace this one with RefSources in TypeVar?
		Offset offset(const TypeVar *var) const;

		// Type chain.
		TypeChain chain;

		// VTable for this type. Not too useful if we're a value type, but maintained anyway for simplicity.
		VTable vtable;

	protected:
		virtual void output(wostream &to) const;

		// Called before the first time any information is wanted.
		// NOTE: Inheritance information is assumed to be set up in a non-lazy way!
		virtual void lazyLoad();

		// Call to inhibit lazy-loading until later.
		void allowLazyLoad(bool v);

	private:
		// Members.
		typedef hash_map<String, Auto<Overload> > MemberMap;
		MemberMap members;

		// Validate parameters to added members.
		void validate(NameOverload *m);

		// Our parent type's size.
		Size superSize() const;

		// Our size (including base classes). If it is zero, we need to re-compute it!
		Size mySize;

		// Variable layout.
		TypeLayout layout;

		// Loaded the lazy parts?
		bool lazyLoaded;

		// Loading the lazy parts?
		bool lazyLoading;

		// Ensure that any lazy-loaded parts are loaded.
		void ensureLoaded();

		// Init (shared parts of constructors).
		void init();

		// Update the need for virtual calls for all members.
		void updateVirtual();
		void updateVirtual(Overload *o);

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
