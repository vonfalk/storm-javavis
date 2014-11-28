#pragma once
#include "Named.h"
#include "Scope.h"
#include "Overload.h"
#include "Package.h"
#include "Code/Value.h"

namespace storm {

	/**
	 * Define different properties for a type.
	 */
	enum TypeFlags {
		// Regular type.
		typeClass = 0x01,

		// Is it a value type (does not inherit from Object).
		typeValue = 0x02,
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
		// If size == 0, it will be automatically computed based on members. Size
		// is mainly used to denote built-in classes.
		Type(const String &name, TypeFlags flags, nat size = 0);
		~Type();

		// Create the first type instance, as an instance of itself.
		static Type *createType(Engine &engine, const String &name, TypeFlags flags);

		static const String CTOR;

		// Owning engine. Currently this has to be the first member in Type. Otherwise modify createType.
		Engine &engine;

		// Type flags.
		const TypeFlags flags;

		// Get the size of this type.
		nat size() const;

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
		virtual code::Value destructor() const;

	protected:
		virtual void output(wostream &to) const;

	private:
		// Members.
		typedef hash_map<String, Auto<Overload> > MemberMap;
		MemberMap members;

		// Super types (when inheritance is used). The last element is us, next to last our super and so on.
		vector<Type *> superTypes;

		// Known child types.
		set<Type *> children;

		// Validate parameters to added members.
		void validate(NameOverload *m);

		// Our parent type's size.
		nat superSize() const;

		// Fixed size (as a built-in type)?
		nat fixedSize;

		// Our size (including base classes). If it is zero, we need to re-compute it!
		mutable nat mySize;

	};

}
