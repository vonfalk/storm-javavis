#pragma once
#include "Named.h"
#include "Scope.h"
#include "Overload.h"

namespace storm {

	/**
	 * Define different properties for a type.
	 */
	enum TypeFlags {
		// Regular type.
		typeClass = 0x00,

		// Is it a value type (does not inherit from Object).
		typeValue = 0x01,
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
	class Type : public Named, public Scope {
	public:
		Type(const String &name, TypeFlags flags);
		~Type();

		static const String CTOR;

		// Type flags.
		const TypeFlags flags;

		// Get the size of this type.
		virtual nat size() const;

		// Set the parent scope (done automatically by Package).
		void setParentScope(Scope *parent);

		// Set parent type. The parent type has to have the same type parameters as this one.
		void setSuper(Type *super);

		// Add new members.
		void add(NameOverload *m);

		// Define some ordering between the types.
		bool operator <(const Type &o) const;

	protected:
		virtual void output(wostream &to) const;
		virtual Named *findHere(const Name &name);

	private:
		// Members.
		typedef hash_map<String, Overload*> MemberMap;
		MemberMap members;

		// Super type (when inheritance is used).
		Type *superType;

		// Validate parameters to added members.
		void validate(NameOverload *m);
	};

}
