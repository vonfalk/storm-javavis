#pragma once
#include "Shared/Map.h"
#include "Thread.h"
#include "Package.h"
#include "Exception.h"

#include "Rule.h"
#include "Option.h"

namespace storm {
	namespace syntax {
		// TODO: This should be moved to core.lang when the old parser is removed!
		// Remember to change in ParserTemplate as well!
		STORM_PKG(lang.syntax);

		/**
		 * Base class for the templated parser in Storm. In Storm, Parser<T> is to be
		 * instantiated. This is not possible from C++ as the syntax types are not present in
		 * C++. Therefore, the interface differs a bit (C++ loses some of the type safety Storm gets
		 * for free).
		 *
		 * TODO? We may want to run this on any thread. The only issue there is the interfacing with
		 * the compiler's type system, which should be relatively safe anyway (but breaks the
		 * threading model).
		 */
		class ParserBase : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create. Create via Parser from C++ as the type has to be set properly for this to work.
			ParserBase();

			// Add a package containing syntax definition (not recursive).
			void STORM_FN addSyntax(Par<Package> pkg);
			void STORM_FN addSyntax(Par<ArrayP<Package>> pkg);

			// Get the root rule.
			Rule *STORM_FN rootRule() const;

			// Parse a string. Returns an iterator to the first character not matched.
			Str::Iter STORM_FN parse(Par<Str> str, SrcPos from);
			Str::Iter STORM_FN parse(Par<Str> str, SrcPos from, Str::Iter start);
			Str::Iter STORM_FN parse(Par<Str> str, Par<Url> file);
			Str::Iter STORM_FN parse(Par<Str> str, Par<Url> file, Str::Iter start);

			/**
			 * Operations on the last parse.
			 */

			// Found any errors? If Str::Iter is not end, this is true.
			Bool STORM_FN hasError();

			// Get the error.
			SyntaxError error() const;

			// Throw the error.
			void STORM_FN throwError() const;

			// Get the error message.
			Str *STORM_FN errorMsg() const;


		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// All rules we know of so far, and their options.
			Auto<MAP_PP(Rule, ArrayP<Option>)> rules;

			// Get the options for a rule (creates it if it does not yet exist). Returns borrowed ptr!
			ArrayP<Option> *options(Par<Rule> rule);
		};


		/**
		 * C++ instantiation. Should be compatible enough to allow instances to be passed via
		 * ParserBase to Storm. The preprocessor does not know of the name Parser and can not bridge
		 * the differences between its usage in C++ and in Storm.
		 *
		 * Make sure not to introduce any additional data members here, as they might get lost
		 * during accidental interfacing with Storm. This class should only contain functions that
		 * completes the interface from the C++ side.
		 */
		class Parser : public ParserBase {
		public:
			// TODO!
		};

	}
}
