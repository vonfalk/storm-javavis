#pragma once
#include "Core/Array.h"
#include "Core/Map.h"
#include "RuleInfo.h"
#include "Stack.h"
#include "Compiler/Syntax/ParserBackend.h"

namespace storm {
	namespace syntax {
		namespace ll {
			STORM_PKG(lang.bnf.ll);

			/**
			 * The LL parser backend. This is an LL(n) parser (i.e. recursive descent) that rewrites
			 * left recursion on-the-fly as needed.
			 */
			class Parser : public ParserBackend {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR Parser();

			protected:
				// Add syntax.
				virtual void add(Rule *rule);
				virtual void add(ProductionType *prod);

				// Does this parser contain the same syntax as 'o'?
				virtual Bool sameSyntax(ParserBackend *o);

				// Parse a string. Returns 'true' if we found some match.
				virtual Bool parse(Rule *root, Str *str, Url *file, Str::Iter start);

				// Parse a string, doing error recovery. Only call 'infoTree' after parsing in this
				// manenr, as the resulting syntax tree is not neccessarily complete.
				virtual InfoErrors parseApprox(Rule *root, Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) ctx);

				// Clear all state.
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

				// Get the syntax tree. Only for C++, in Storm we know the exact subtype we will generate!
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

				/**
				 * Grammar
				 */

				// Remember the rules. Each one is given an index.
				Array<RuleInfo *> *rules;

				// Remember which rules belong where.
				Map<Rule *, Nat> *ruleId;

				// Syntax prepared for parsing?
				Bool syntaxPrepared;

				// Prepare the syntax for parsing if needed.
				void prepare();

				// Find a rule.
				MAYBE(RuleInfo *) findRule(Rule *r);

				// Internal parse function.
				Bool parse(ProductionIter iter, Str *str, Nat pos);

				// Parse a token known to refer to a regex.
				void parseRegex(RegexToken *regex, StackItem *&top, Str *str);

				// Parse a token known to refer to a rule.
				void parseRule(RuleToken *rule, StackItem *&top, Str *str);
			};

		}
	}
}
