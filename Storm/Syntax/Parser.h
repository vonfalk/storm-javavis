#pragma once
#include "Shared/Map.h"
#include "Thread.h"
#include "Package.h"
#include "Exception.h"

#include "Rule.h"
#include "Option.h"
#include "State.h"

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
		 * The parser implemented here is based on the parser described by Jay Earley in An
		 * Efficient Context-Free Parsing Algoirthm, University of California, Berkeley, California.
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

			// Found any errors? If Str::Iter is not end, this is true. Note that even if we have an
			// error, it could be possible to extract a tree!
			Bool STORM_FN hasError();

			// Get the error.
			SyntaxError error() const;

			// Throw the error.
			void STORM_FN throwError() const;

			// Get the error message.
			Str *STORM_FN errorMsg() const;

			// Get the number of entries used.
			Nat STORM_FN stateCount() const;

			// Get the number of bytes used.
			Nat STORM_FN byteCount() const;

			// Get the syntax tree (only for C++). In Storm we will know what type of tree we will generate!
			Object *tree() const;

		protected:
			// Output.
			virtual void output(wostream &to) const;

		private:
			// All rules we know of so far, and their options.
			Auto<MAP_PP(Rule, ArrayP<Option>)> rules;

			// Remember which rules matches the empty string.
			Auto<MAP_PV(Rule, Bool)> emptyCache;

			// Get the options for a rule (creates it if it does not yet exist). Returns borrowed ptr!
			ArrayP<Option> *options(Par<Rule> rule);

			// Allocator for State nodes.
			StateAlloc stateAlloc;

			// Parsed string.
			Par<Str> src;

			// Position of the src string.
			SrcPos srcPos;

			// Root option.
			Auto<Option> rootOption;

			// Steps. Each step corresponds to a character in the input string + an additional step at the end.
			vector<StateSet> steps;

			// Last state containing a finish step.
			nat lastFinish;

			// Take note of the thread used for objects derived from Type.
			Thread *typeThread;

			// Offset of 'pos' in all node objects.
			int posOffset;

			// Process a single step.
			bool process(nat step);

			// Predictor, completer and scanner as described in the paper. The StateSet should be
			// the current step, ie. the set in which 'state' belongs.
			void predictor(nat step, State *state);
			void completer(nat step, State *state);
			void scanner(nat step, State *state);

			// Does 'rule' match an empty string?
			bool matchesEmpty(const Auto<Rule> &rule);
			bool matchesEmpty(Par<Option> opt);
			bool matchesEmpty(Par<Token> tok);

			// Find the last step which is not empty.
			nat lastStep() const;

			// Find the finishing state. Returns null if none.
			State *finish() const;

			// Create a tree for the option which ends at 'state'.
			Object *tree(State *state) const;

			// Create an object of the type suitable for the rule in 'state'.
			Object *allocTreeNode(State *state) const;

			// Reverse arrays in a node.
			void reverseTreeNode(State *state, Object *node) const;

			// Get all rules in progress for a specific state (for error messages).
			map<String, set<String>> inProgress(const StateSet &step) const;
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
