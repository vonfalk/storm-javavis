#pragma once
#include "SyntaxOption.h"

namespace storm {

	/**
	 * Describes a syntax type. A type consists of zero or more syntax rules. A syntax
	 * type may then be matched to one of the rules present.
	 */
	class SyntaxRule : public Printable, NoCopy {
	public:
		// Create a new syntax type. If !owner, we will not delete our options on destruction.
		SyntaxRule(const String &name, bool owner = true);

		~SyntaxRule();

		// Add a rule. Takes ownership of the pointer.
		void add(SyntaxOption *rule);

		// Options access.
		inline nat size() const { return options.size(); }
		inline SyntaxOption* operator[] (nat i) { return options[i]; }

		// Get our name.
		inline const String &name() const { return rName; }

		// Formal parameters.
		struct Param {
			String type, name;
		};

		// Parameters for this rule.
		vector<Param> params;

		// Where were we declared? "unknown()" if not declared.
		SrcPos declared;

		// Copy our declaration to another.
		void copyDeclaration(const SyntaxRule &o);

	protected:
		virtual void output(std::wostream &to) const;

	private:
		// The name of this type.
		String rName;

		// List of all options associated to this type.
		vector<SyntaxOption*> options;

		// Owner?
		bool owner;
	};

	wostream &operator <<(wostream &to, const SyntaxRule::Param &p);
}
