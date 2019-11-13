#pragma once
#include "Compiler/NamedThread.h"
#include "Compiler/Syntax/ParserBackend.h"
#include "Compiler/Syntax/Node.h"
#include "Compiler/Syntax/InfoNode.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Production.h"
#include "Compiler/Syntax/Earley/State.h"
#include "Core/Array.h"
#include "Core/Map.h"
#include "OS/FnCall.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace syntax {
		namespace earley {
			STORM_PKG(core.lang.earley);

			/**
			 * The parser here is the main parser in Storm and is based on the parser described by Jay
			 * Earley in 'An Efficient Context-Free Parsing Algorithm'.
			 *
			 * TODO: Make it possible to parse things on other threads than the compiler thread.
			 */
			class Parser : public ParserBackend {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR Parser();

			protected:
				// Add a package containing syntax definitions (not recursive).
				virtual void add(Rule *rule);
				virtual void add(ProductionType *production);

				// Does this parser contain the same syntax as 'o'?
				virtual Bool sameSyntax(ParserBackend *o);

				// Parse a string. Returns 'true' if we found some match.
				virtual Bool parse(Rule *root, Str *str, Url *file, Str::Iter start);

				// Clear all parse-related information. Included packages are retained.
				virtual void clear();

				/**
				 * Operations on the last parse.
				 */

				// Found any errors? If Str::Iter is not end, this is always true. Note that even if we
				// have an error, it could be possible to extract a tree!
				virtual Bool hasError() const;

				// Is it possible to extract a syntax tree? (equivalent to the return value of 'parse').
				virtual Bool hasTree() const;

				// Return an iterator after the last matched character, or the start of the string if no
				// match could be made.
				virtual Str::Iter matchEnd() const;

				// Get the error message.
				virtual Str *errorMsg() const;

				// Get the error position.
				virtual SrcPos errorPos() const;

				// Get the syntax tree.
				virtual Node *tree() const;

				// Get the generic syntax tree.
				virtual InfoNode *infoTree() const;


				/**
				 * Performance inspection:
				 */

				// Get the number of states used.
				virtual Nat stateCount() const;

				// Get the number of bytes used.
				virtual Nat byteCount() const;

			private:
				// Information about a rule.
				class RuleInfo {
					STORM_VALUE;
				public:
					STORM_CTOR RuleInfo();

					// All productions for this rule. May be null.
					Array<Production *> *productions;

					// Can this rule match the empty string? 0 = no, 1 = yes, >=2 don't know yet.
					Byte matchesNull;

					// Add a production. Handles the case where 'productions' is null.
					void push(Production *p);
				};

				// All rules we know of so far, and their options.
				Map<Rule *, RuleInfo> *rules;

				// Parsed source string.
				Str *src;

				// Current file and start position.
				MAYBE(Url *) srcFile;
				Nat srcOffset;

				// Root production.
				Production *rootProd;

				// Steps. Each step corresponds to a character in the input string (including an
				// implicit end of string character).
				GcArray<StateSet *> *steps;

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

				// Create a tree for the production ending in 'end'.
				Node *tree(StatePtr end) const;

				// Allocate a tree node.
				Node *allocNode(const State &from, Nat endStep) const;

				// Reverse all arrays in a node.
				void reverseNode(Node *node) const;

				// Empty string used in the info tree.
				Str *emptyString;

				// Create a tree for the production ending in 'end'.
				InfoNode *infoTree(StatePtr end) const;

				// Get a State from a StatePtr.
				const State &state(const StatePtr &p) const;

				// State set needs to access 'state()'
				friend class StateSet;
			};

		}
	}
}
