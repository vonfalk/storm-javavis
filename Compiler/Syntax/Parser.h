#pragma once
#include "Compiler/NamedThread.h"
#include "Core/Array.h"
#include "Core/Map.h"
#include "OS/FnCall.h"
#include "Node.h"
#include "Rule.h"
#include "Production.h"
#include "State.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace syntax {
		STORM_PKG(core.lang);

		/**
		 * Base class for the templated parser in Storm. In Storm, Parser<T> is to be
		 * instantiated. This is not possible from C++ as the syntax types are not present in
		 * C++. Therefore, the interface differs a bit (C++ loses some type safety compared to
		 * Storm).
		 *
		 * The parser here is the main parser in Storm and is based on the parser described by Jay
		 * Earley in An Efficient Context-Free Parsing Algorithm.
		 *
		 * TODO: Make it possible to parse things on other threads than the compiler thread.
		 */
		class ParserBase : public ObjectOn<Compiler> {
			STORM_CLASS;
		protected:
			// Create. Create via Parser from C++ as the type has to be set properly for this to work.
			ParserBase();

		public:
			// Add a package containing syntax definitions (not recursive).
			void STORM_FN addSyntax(Package *pkg);
			void STORM_FN addSyntax(Array<Package *> *pkg);

			// Get the root rule.
			Rule *STORM_FN root() const;

			// Parse a string. Returns 'true' if we found some match.
			Bool STORM_FN parse(Str *str, Url *file);
			Bool STORM_FN parse(Str *str, Url *file, Str::Iter start);

			/**
			 * Operations on the last parse.
			 */

			// Found any errors? If Str::Iter is not end, this is always true. Note that even if we
			// have an error, it could be possible to extract a tree!
			Bool STORM_FN hasError() const;

			// Is it possible to extract a syntax tree? (equivalent to the return value of 'parse').
			Bool STORM_FN hasTree() const;

			// Return an iterator after the last matched character, or the start of the string if no
			// match could be made.
			Str::Iter STORM_FN matchEnd() const;

			// Get the error (present if 'hasError' is true).
			SyntaxError error() const;

			// Throw error if it is present.
			void STORM_FN throwError() const;

			// Get the error message.
			Str *STORM_FN errorMsg() const;

			// Get the syntax tree. Only for C++, in Storm we know the exact subtype we will generate!
			Node *tree() const;

			// Output.
			void STORM_FN toS(StrBuf *to) const;

			/**
			 * Performance inspection:
			 */

			// Get the number of states used.
			Nat STORM_FN stateCount() const;

			// Get the number of bytes used.
			Nat STORM_FN byteCount() const;

		private:
			// Information about a rule.
			class RuleInfo {
				STORM_VALUE;
			public:
				STORM_CTOR RuleInfo();

				// All productions for this rule. May be null.
				Array<Production *> *productions;

				// Can this rule match the empty string? 0 = no, 1 = yes, >2 don't know yet.
				Byte matchesNull;

				// Add a production. Handles the case where 'productions' is null.
				void push(Production *p);
			};

			// All rules we know of so far, and their options.
			Map<Rule *, RuleInfo> *rules;

			// Parsed source string.
			Str *src;

			// Initial position in 'src'. '.offset' contains the first char parsed (this corresponds to states[0]).
			SrcPos srcPos;

			// Root production.
			Production *rootProd;

			// Steps. Each step corresponds to a character in the input string (including an
			// implicit end of string character).
			Array<StateSet *> *steps;

			// Last state containing a finish step (initialized to something >= states.count).
			nat lastFinish;

			// Process a single step. Returns true if we found an accepting state here.
			bool process(nat step);

			// Predictor, completer and scanner as described in the paper. The StateSet should be
			// the current step, ie. the set in which 'state' belongs.
			void predictor(StatePtr ptr, const State &state);
			void completer(StatePtr ptr, const State &state);
			void scanner(StatePtr ptr, const State &state);

			// Does 'rule' match an empty string?
			bool matchesEmpty(Rule *r);
			bool matchesEmpty(RuleInfo &info);
			bool matchesEmpty(Production *p);
			bool matchesEmpty(Token *t);

			// Find the last step which is not empty.
			nat lastStep() const;

			// Find the finishing state (the last one if there are more).
			const State *finish() const;

			// Find all rules and productions in progress for a given state.
			Map<Str *, StrBuf *> *inProgress(const StateSet &step) const;

			// Create a tree for the production ending in 'state'.
			Node *tree(StatePtr end) const;

			// Allocate a tree node.
			Node *allocNode(const State &from) const;

			// Reverse all arrays in a node.
			void reverseNode(Node *node) const;

			// Get a State from a StatePtr.
			const State &state(const StatePtr &p) const;

			// State set needs to access 'state()'
			friend class StateSet;
		};


		/**
		 * C++ instantiation, compatible enough to pass on to Storm. This class is not visible to
		 * the preprocessor, so use ParserBase for that.
		 *
		 * Make sure to not introduce any additional data members here, as they might get lost
		 * during accidental interfacing with Storm. This class should only contain functions that
		 * completes the interface from the C++ side.
		 */
		class Parser : public ParserBase {
		public:
			// Create a parser parsing a specific type.
			static Parser *create(Rule *root);

			// Create a parser parsing the type named 'name' in 'pkg'.
			static Parser *create(Package *pkg, const wchar *name);

			// Transform syntax nodes. See limitations for 'transformNode<>'.
			template <class R>
			R *transform() {
				return transformNode<R>(tree());
			}

			template <class R, class P>
			R *transform(P par) {
				return transformNode<R, P>(tree(), par);
			}

		private:
			// Use 'create'.
			Parser();
		};


		// Declare the template. This does not match the above declaration, so use ParserBase to
		// store Parser instances in classes!
		STORM_TEMPLATE(Parser, createParser);

	}
}
