#pragma once
#include "Type.h"
#include "Parse.h"
#include "Token.h"
#include "Shared/EnginePtr.h"

namespace storm {
	namespace syntax {

		/**
		 * Iterator for an Option.
		 *
		 * This iterator does not follow the convention in Storm, as options are not
		 * deterministic. This indeterminism is due to the ?,+ and *. However, it is only ever
		 * possible to enter two new states from an existing state, which is why nextA and nextB
		 * suffices.
		 *
		 * Note: the implementation of OptionIter breaks the thread-safety a bit, as it reads from
		 * the Option from a thread that is not neccessarily the Compiler thread. Should not be too
		 * bad, but is worth noticing.
		 */
		class OptionIter {
			friend class Option;
			STORM_VALUE;
		public:
			// Are we at the end of the iteration?
			Bool STORM_FN end() const;

			// Get the two iterators which follows this. Returns an iterator where 'end' == true if
			// the operation is not feasible.
			OptionIter STORM_FN nextA() const;
			OptionIter STORM_FN nextB() const;

			// Compare for equality.
			Bool STORM_FN operator ==(const OptionIter &o) const;
			Bool STORM_FN operator !=(const OptionIter &o) const;

		private:
			// Create.
			OptionIter(Par<Option> option, Nat pos = 0);

			// The option we own. We only keep a weak pointer here, as we do not want to incur any
			// overhead when copying or creating one of these objects (this is don *a lot* of times
			// when parsing).
			Option *option;

			// Position into the option's tokens.
			Nat pos;
		};

		// Output.
		wostream &operator <<(wostream &to, const OptionIter &o);
		Str *STORM_ENGINE_FN toS(EnginePtr e, OptionIter o);

		/**
		 * Syntax option.
		 *
		 * This is represented as a type, which inherits from a Rule type. This class will override
		 * the 'eval' member of the parent type.
		 */
		class Option : public Type {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Option(Par<Str> name, Par<OptionDecl> decl, MAYBE(Par<Rule>) delim, Scope scope);

			// Declared at.
			STORM_VAR SrcPos pos;

			// Scope.
			STORM_VAR Scope scope;

			// Tokens.
			STORM_VAR Auto<ArrayP<Token>> tokens;

			// Priority.
			STORM_VAR Int priority;

			// Repeat logic.
			STORM_VAR Nat repStart;
			STORM_VAR Nat repEnd;
			STORM_VAR RepType repType;

			// Get an iterator into this option.
			OptionIter STORM_FN firstA();
			OptionIter STORM_FN firstB();

			// Get owner rule.
			Rule *STORM_FN rule() const;

			// Load contents.
			virtual Bool STORM_FN loadAll();

		protected:
			virtual void output(wostream &to) const;

		private:
			void outputRepEnd(wostream &to) const;

			// Load tokens from a declaration.
			void loadTokens(Par<OptionDecl> decl, Par<Rule> delim);

			// Add a single token to us.
			void addToken(Par<TokenDecl> decl, Par<Rule> delim, const SrcPos &optionPos);
		};


	}
}
