#pragma once
#include "Core/Array.h"
#include "Core/Map.h"
#include "RuleInfo.h"
#include "Stack.h"
#include "Tree.h"
#include "Compiler/Syntax/ParserBackend.h"

namespace storm {
	namespace syntax {
		namespace gll {
			STORM_PKG(lang.bnf.gll);

			/**
			 * The GLL parser backend. This is an LL(n) parser (i.e. recursive descent) that rewrites
			 * left recursion on-the-fly as needed.
			 *
			 * Based on http://www.cs.rhul.ac.uk/research/languages/csle/GLLparsers.html
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
				 * Cache.
				 */

				// GcType of an array of TreeItem, so we don't need to look it up all the time.
				const GcType *treeArrayType;


				/**
				 * Grammar
				 */

				// Remember the rules. Each one is given an index.
				Array<RuleInfo *> *rules;

				// Remember which rules belong where.
				Map<Rule *, Nat> *ruleId;

				// ID of each production in a rule. Updated when productions are sorted.
				Map<Production *, Nat> *prodId;

				// Syntax prepared for parsing?
				Bool syntaxPrepared;

				// Prepare the syntax for parsing if needed, and setup other state.
				void prepare(Str *str, Url *file);

				// Find a rule.
				MAYBE(RuleInfo *) findRule(Rule *r) const;

				// Internal parse function.
				void parse(RuleInfo *rule, Nat offset);

				// Parse a stack item as far as we can. Produce new stacks in the PQ as needed.
				void parseStack(StackItem *top);

				// "reduce" the current stack (this is not LL terminology, but it is similar enough
				// to reduce in LR parsers).
				void parseReduce(StackItem *top);

				/**
				 * Parser state.
				 */

				// URL of the string currently being matched.
				Url *url;

				// String currently being matched.
				Str *src;

				// Priority queue of current stack-tops. Roughly corresponds to the R set in the
				// paper. We manage the priority queue ourselves to be able to inline calls to
				// operator < and manage the size slightly different compared to PQueue (we allocate
				// more frivolously for example).
				GcArray<StackItem *> *pq;

				// Production sequence number. Used to ensure that the match with the highest
				// priority is processed first in the priority queue.
				Nat stackId;

				// Initialize the priority queue for a parse.
				void pqInit();

				// Push an element on the priority queue.
				void pqPush(StackItem *item);

				// Push a new item, creating the item as well. 'first' indicates if this is the
				// first state in a production or not.
				void pqPush(StackItem *prev, ProductionIter iter, Nat inputPos, Bool first);

				// Pop the top element of the priority queue. Returns null if empty.
				StackItem *pqPop();

				// Create a stack item.
				StackItem *create(StackItem *prev, ProductionIter iter, Nat inputPos, Bool first);

				/**
				 * Parser results.
				 */

				// The entire syntax tree, if successful match.
				Tree *matchTree;

				// First and last position of the match.
				Nat matchFirst;
				Nat matchLast;

				// Rule and production ID matched.
				Nat matchRule;
				Nat matchProd;


				/**
				 * Extract a parse tree.
				 */

				// Create a node for a nonterminal.
				Node *tree(RuleInfo *rule, Tree *root, Nat firstPos, Nat lastPos) const;

				// Create a node for a terminal or a non-terminal, and save it inside a node.
				void setNode(Node *into, Token *token, TreePart part, Nat lastPos) const;

				// Allocate a node.
				Node *allocNode(Production *from, Nat start, Nat end) const;

				// Create a node for an info-tree.
				InfoNode *infoTree(RuleInfo *rule, Tree *root, Nat lastPos) const;
			};

		}
	}
}
