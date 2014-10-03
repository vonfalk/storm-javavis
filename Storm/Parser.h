#pragma once

#include "SyntaxType.h"
#include "SyntaxRule.h"
#include "Exception.h"

namespace storm {

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
	 * A set of packages making up all the syntax needed for parsing.
	 */
	class SyntaxSet : NoCopy {
		friend class Parser;
	public:
		// Add syntax from a package.
		void add(Package &pkg);

		// Parse a string from 'start', trying to match the 'root' rule.
		// Returns the index of the first non-matched character. Throws an exception on failure.
		nat parse(const String &root, const String &str, nat start = 0);

	private:
		// All syntax definitions.
		hash_map<String, SyntaxType*> syntax;
	};


	/**
	 * The actual parser. Create for a specific string, call parse.
	 * Implemented from article: An Efficient Context-Free Parsing Algorithm
	 * by Jay Earley, University of Califormnia, Berkeley, California
	 */
	class Parser : NoCopy {
	public:
		// Create the parser, reads syntax from 'set'. 'set' is expected
		// to outlive this object.
		Parser(SyntaxSet &set, const String &src);

		// Parse the string previously given from 'start'. Returns the first index not matched.
		nat parse(const String &rootType, nat start = 0);

		/**
		 * Operations on the last parse.
		 */

		// Is there any error information to retrieve?
		bool hasError() const;

		// Get an error message on the parsing. This examines what
		// went wrong on the last step, maybe beyond the length indicated
		// by 'parse'.
		SyntaxError error(const Path &file) const;

	private:
		// Syntax source.
		SyntaxSet &syntax;

		// Source string.
		const String &src;

		/**
		 * A single state in the parsing.
		 */
		class State : public Printable {
		public:
			// Position in a rule.
			RuleIter pos;

			// In which step was this rule instantiated?
			nat from;

			// Lookahead, do we need this?
			String lookahead;

			// Create empty state.
			State() : from(0) {}

			// Create a state.
			State(const RuleIter &ri, nat from, const String &l) : pos(ri), from(from), lookahead(l) {}

			// Equality.
			inline bool operator ==(const State &o) const {
				return pos == o.pos
					&& from == o.from
					&& lookahead == o.lookahead;
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

			// Does this state represent a state where we are done?
			bool finish(const SyntaxRule *rootRule) const;

		protected:
			virtual void output(wostream &to) const;
		};

		/**
		 * Define a state set.
		 */
		class StateSet : public vector<State> {
		public:
			void insert(const State &s);
		};

		// State sets.
		vector<StateSet> steps;

		/**
		 * Helpers.
		 */

		// The root rule.
		SyntaxRule rootRule;

		// Find the last step wich is not empty.
		nat lastStep() const;

		// Find possible type completions for a step.
		set<String> typeCompletions(const StateSet &states) const;

		// Find possible regex completions for a step.
		set<String> regexCompletions(const StateSet &states) const;

		/**
		 * Parsing.
		 */

		// Process the set at index 'set'. Returns true if a 'finish' state is seen.
		bool process(nat step);

		// Run the predictor on one element in 'set'.
		void predictor(StateSet &set, nat pos, State state);

		// Run the scanner on one element in 'set'.
		void scanner(StateSet &set, nat pos, State state);

		// Run the completer on one element in 'set'.
		void completer(StateSet &set, nat pos, State state);
	};

}
