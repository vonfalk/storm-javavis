#pragma once

#include "SyntaxSet.h"
#include "SyntaxRule.h"
#include "SyntaxOption.h"
#include "SyntaxNode.h"
#include "Exception.h"
#include "Lib/TObject.h"
#include "Utils/PreArray.h"

namespace storm {
	STORM_PKG(core.lang);

	class Package;

	/**
	 * This file contains the BNF parser, which will take a set of rules
	 * as an input along with a string to be parsed. The parser will then
	 * produce the parse tree according to the rules for as much of the
	 * string as possible. The parser will try to parse as much as possible
	 * of the file, possibly terminating before the end of the input.
	 *
	 * Note: The parser will not claim ownership of any rule or type objects.
	 */

	/**
	 * The actual parser. Create for a specific string, call parse.
	 * Implemented from article: An Efficient Context-Free Parsing Algorithm
	 * by Jay Earley, University of Califormnia, Berkeley, California
	 */
	class Parser : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:
		static const nat NO_MATCH = -1;

		// Create the parser, reads syntax from 'set'. 'set' is expected
		// to outlive this object.
		// 'pos' is the start position of 'src'. If 'src' is the entire file, the second
		// variant can also be used.
		STORM_CTOR Parser(Par<SyntaxSet> set, Par<Str> src, const SrcPos &pos);
		STORM_CTOR Parser(Par<SyntaxSet> set, Par<Str> src, Par<Url> file);

		// Parse the string previously given from 'start'. Returns the first index not matched.
		Nat STORM_FN parse(Par<Str> rootType);
		Nat STORM_FN parse(Par<Str> rootType, Nat start);
		nat parse(const String &rootType, nat start = 0);

		// Get the no-match value (for Storm).
		Nat STORM_FN noMatch() const;

		/**
		 * Operations on the last parse.
		 */

		// Is there any error information to retrieve?
		Bool STORM_FN hasError() const;

		// Get an error message on the parsing. This examines what
		// went wrong on the last step, maybe beyond the length indicated
		// by 'parse'.
		SyntaxError error() const;

		// Helper for Storm to throw the error.
		void STORM_FN throwError() const;

		// Get the error string (for Storm).
		Str *STORM_FN errorMsg() const;

		// Generate a syntax tree. Leaves ownership of the tree to the caller.
		// If the last parse was not successfull at all (ie, returned NO_MATCH), returns null.
		// 'file' is for correct SrcRefs in the resulting tree.
		SyntaxNode *tree();

		// Shorthand for generating a tree and transform it into Storm objects. Note 'params' does not own refs.
		// TODO: This transform + transformT is a little bit ugly...
		Object *STORM_FN transform(Par<ArrayP<Object>> params);
		TObject *STORM_FN transformT(Par<ArrayP<TObject>> params);

		Object *transform(const vector<Object *> &params = vector<Object *>());

	private:
		// Syntax source.
		Auto<SyntaxSet> syntax;

		// Source string.
		Auto<Str> srcStr;
		const String &src;

		// Start of 'src'.
		const SrcPos srcPos;

		/**
		 * Reference to a state.
		 */
		class StatePtr : public Printable {
		public:
			// Invalid ptr.
			inline StatePtr() : step(invalid), id(invalid) {}
			// Reference something.
			inline StatePtr(nat step, nat id) : step(step), id(id) {}

			// Step.
			nat step;

			// Id in that step.
			nat id;

			// Invalid id.
			static const nat invalid = -1;

			// Valid reference?
			inline bool valid() const {
				return step != invalid && id != invalid;
			}
		protected:
			virtual void output(wostream &to) const;
		};


		/**
		 * A single state in the parsing.
		 * With the 'prev' and 'completed' pointers, these state nodes will
		 * form a linked path from the finished state to the beginning of the
		 * parse. Each state representing the first installment of a node has
		 * both of these set to null.
		 * Note that 'prev' and 'completed' are not included in the == comparision.
		 * This is so that grammars that have multiple valid parses shall still
		 * give a single result.
		 */
		class State : public Printable {
		public:
			// Position in a rule.
			OptionIter pos;

			// In which step was this rule instantiated?
			nat from;

			// Lookahead, do we need this?
			// String lookahead;

			// Last instance of this state. Ie. with pos == pos - 1.
			StatePtr prev;

			// What state was completed to make this advance (if any)?
			StatePtr completed;

			// Create empty state.
			State() : from(0) {}

			// Create a state.
			State(const OptionIter &ri, nat from,
				const StatePtr &prev, const StatePtr &completed)
				: pos(ri), from(from), prev(prev), completed(completed) {}

			// Equality.
			inline bool operator ==(const State &o) const {
				return pos == o.pos
					&& from == o.from;
			}
			inline bool operator !=(const State &o) const {
				return !(*this == o);
			}

			// Does 'pos' point to a rule with the name 'name'?
			bool isRule(const String &name) const;

			// Is the next token a rule?
			bool isRule() const;

			// What is the name of the next rule, assumes isRule();
			const String &tokenRule() const;

			// Is the next token a regex?
			bool isRegex() const;

			// What is the content of the next regex, assumes isRegex();
			const Regex &tokenRegex() const;

			// Bind the next token to something?
			bool bindToken() const;

			// Bind the next token to this variable.
			const String &bindTokenTo() const;

			// Invoke some method on the next token?
			bool invokeToken() const;

			// Invoke which metod on the next token.
			const String &invokeOnToken() const;

			// Does this state represent a state where we are done?
			bool finish(const SyntaxOption *rootOption) const;

			// Get the priority of this rule.
			inline int priority() const { return pos.option().priority; }

		protected:
			virtual void output(wostream &to) const;
		};

		// Create some states.
		State firstState();
		State completedState(const OptionIter &ri, nat from, const StatePtr &prev, const StatePtr &completedBy);
		State predictedState(const OptionIter &ri, nat from);
		State scannedState(const OptionIter &ri, nat from, const StatePtr &prev);

		/**
		 * Define a state set.
		 */
		class StateSet : public vector<State> {
		public:
			// Does not insert invalid positions.
			// void insert(const State &s); // use 'insert' in 'Parser' instead.
		};

		// State sets.
		vector<StateSet> steps;

		/**
		 * Helpers.
		 */

		// Insert a state if it is valid and not duplicated. May modify the 'completed by' to
		// correctly respond to priority of the options.
		void insert(StateSet &into, const State &state);

		// Execution order of two states.
		enum ExecOrder {
			before,
			after,
			none,
		};

		// Determine the order in which the states 'a' and 'b' would have been executed if we
		// would follow the semantics of a PEG parser with respect to the priorities of the rules.
		// Rules with equal priority are considered to have no relation.
		ExecOrder execOrder(const StatePtr &a, const StatePtr &b);

		// Size of the PreArray. Should be large enough to accommodate the common length of rules
		// to not lose too much performance.
		static const nat preStates = 10;
		typedef PreArray<StatePtr, preStates> StateArray;

		// Find all previous states of the given state. The given state is _not_ included.
		StateArray prevStates(const StatePtr &end);

		// The root rule.
		SyntaxOption rootOption;

		// Find the finish state.
		StatePtr finish() const;

		// Find the last step wich is not empty.
		nat lastStep() const;

		// Find possible type completions for a step.
		set<String> typeCompletions(const StateSet &states) const;

		// Find possible regex completions for a step.
		set<String> regexCompletions(const StateSet &states) const;

		// Find currently used rules.
		set<String> rulesInProgress(const StateSet &states) const;

		// Get a state from a StatePtr. Note that this reference may be invalidated
		// as soon as something is added to a step!
		State &state(const StatePtr &ptr);

		/**
		 * Parsing.
		 */

		// Process the set at index 'set'. Returns true if a 'finish' state is seen.
		bool process(nat step);

		// Run the predictor on one element in 'set'.
		void predictor(StateSet &set, State state, StatePtr ptr);

		// Run the scanner on one element in 'set'.
		void scanner(StateSet &set, State state, StatePtr ptr);

		// Run the completer on one element in 'set'. Returns true
		// if new states were inserted in the current 'set'.
		void completer(StateSet &set, State state, StatePtr ptr);

		// Determine if the rule 'r' may match the empty string.
		// This uses a cache, and is used in 'predictor' to allow rules matching empty strings.
		bool matchesEmpty(SyntaxRule &rule);

		// A single option?
		bool matchesEmpty(SyntaxOption *option);

		// A single token?
		bool matchesEmpty(SyntaxToken *token);

		// Cache to 'matchesEmpty'.
		typedef hash_map<SyntaxRule *, bool> EmptyCache;
		EmptyCache emptyCache;

		/**
		 * Result extraction.
		 */

		// Extract the syntax tree, starting at 'ptr'.
		SyntaxNode *tree(StatePtr ptr);

		// Print the parse tree (very crude).
		void dbgPrintTree(StatePtr ptr, nat indent = 0);
	};

}
