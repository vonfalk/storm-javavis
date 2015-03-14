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
		SyntaxRule(const String &name, const Scope &scope, bool owner = true);

		~SyntaxRule();

		// Orphan any options.
		void orphanOptions();

		// Add a rule. Takes ownership of the pointer.
		void add(SyntaxOption *rule);

		// Options access.
		inline nat size() const { return options.size(); }
		inline SyntaxOption* operator[] (nat i) { return options[i]; }
		inline const SyntaxOption* operator[] (nat i) const { return options[i]; }

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

		// Scope at declaration site.
		Scope declScope;

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


	/**
	 * A set of syntax rules. Acts like map<name, rule>, but has support for
	 * merges, among other things.
	 * Takes ownership of rules.
	 */
	class SyntaxRules : public Printable, NoCopy {
		typedef hash_map<String, SyntaxRule *> Map;
	public:

		// Iterator type (always const).
		typedef Map::const_iterator iterator;

		// Free contents.
		~SyntaxRules();
		void clear();

		// Add a single rule. Merges with already existing ones if already present.
		// If this rule has a definition, throws an exception.
		void add(SyntaxRule *rule);

		// Add another SyntaxRules.
		void add(const SyntaxRules &o);

		// Get element. Returns null if it does not exist.
		SyntaxRule *operator [](const String &name) const;

		// Iterators
		inline iterator begin() const { return data.begin(); }
		inline iterator end() const { return data.end(); }

		// Empty?
		inline bool empty() const { return data.empty(); }

	protected:
		virtual void output(wostream &to) const;

	private:
		// Contents.
		Map data;

		// Merge two rules.
		void merge(SyntaxRule *to, SyntaxRule *from);
	};
}
