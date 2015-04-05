#pragma once
#include "Name.h"
#include "Lib/TObject.h"
#include "Thread.h"

namespace storm {
	STORM_PKG(core.lang);

	class Package;
	class SyntaxRule;
	class SyntaxOption;

	/**
	 * Store the currently active syntax rules. This class is designed
	 * so that adding a new package is as cheap as possible. This means
	 * that it will not copy the syntax rules or the syntax options.
	 *
	 * TODO: Validate types of rules here?
	 */
	class SyntaxSet : public ObjectOn<Compiler> {
		STORM_CLASS;
	private:
		typedef hash_map<String, SyntaxRule *> RMap;
	public:
		STORM_CTOR SyntaxSet();

		~SyntaxSet();

		// Add syntax from a package. Packages are searched in the
		// order of addition (when that is relevant).
		void add(Package &pkg);

		// Get the SyntaxRule with a specific name.
		SyntaxRule *rule(const String &name);

		// Iterators.
		typedef RMap::const_iterator iterator;
		inline iterator begin() const { return rules.begin(); }
		inline iterator end() const { return rules.end(); }

	protected:
		virtual void output(wostream &to) const;

	private:
		// Added packages.
		set<Package *> added;

		// Syntax rules.
		RMap rules;

		// Add a rule.
		void add(const String &name, SyntaxRule *rule);

		// Find a rule, creates it if it does not already exists.
		SyntaxRule *safeRule(const String &name);
	};

}
