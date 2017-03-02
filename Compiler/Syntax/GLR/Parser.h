#pragma once
#include "Syntax.h"
#include "Table.h"
#include "Stack.h"
#include "BoolSet.h"
#include "Core/Array.h"
#include "Core/Map.h"
#include "Compiler/Syntax/ParserBackend.h"

namespace storm {
	namespace syntax {
		namespace glr {
			STORM_PKG(lang.bnf.glr);

			/**
			 * The GLR parser backend. This parser lazily generates LR-states and uses a GLR parser
			 * to interpret those states.
			 *
			 * TODO: Optimize the storage of stack tops by allocating them in bulk.
			 */
			class Parser : public ParserBackend {
				STORM_CLASS;
			public:
				// Create.
				STORM_CTOR Parser();

			protected:
				// Add syntax.
				virtual void add(Rule *rule);
				virtual void add(ProductionType *type);

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
				 * Syntax related data, persistent between parses.
				 */

				// All syntax.
				Syntax *syntax;

				// The parse table.
				Table *table;

				// One and only empty string used for info trees.
				Str *emptyStr;

				/**
				 * Data cleared between parses.
				 */

				// Stacks for future steps.
				FutureStacks *stacks;

				// Root rule for this parse.
				Nat parseRoot;

				// Source string being parsed.
				Str *source;

				// Url of the source string.
				Url *sourceUrl;

				// Last found stack which accepted the string. Starts with a dummy stack item for
				// the topmost production.
				StackItem *acceptingStack;

				// The last non-empty state set. Used for error reporting.
				Set<StackItem *> *lastSet;

				// The position of 'lastSet'.
				Nat lastPos;

				/**
				 * Data used during parsing.
				 */

				// Current position in the input.
				Nat currentPos;

				// All states which have been processed in this position so far.
				BoolSet *visited;

				/**
				 * Member functions.
				 */

				// Compute the start item-set for a rule.
				ItemSet startSet(Rule *rule);

				// Find the start state for for a rule.
				StackItem *startState(Nat pos, Rule *rule);

				// Add 'production' and all other productions which may complete this production to
				// 'alwaysReduce'.
				void findAlwaysReduce(Nat production);

				// Clear all data derived from the syntax.
				void clearSyntax();

				/**
				 * Parsing functions. Based on the paper "Parser Generation for Interactive
				 * Environments" by Jan Renkers.
				 */

				// Act on all states until we're done.
				void actor(Nat pos, Set<StackItem *> *states);

				// Data passed to the reduction actor.
				struct ActorEnv {
					// Current state.
					State *state;

					// Top of stack before reductions started.
					StackItem *stack;
				};

				// Perform actions required for a state.
				void actorShift(const ActorEnv &env);
				void actorReduce(const ActorEnv &env, StackItem *through);
				void doReduce(const ActorEnv &env, Nat production, StackItem *through);

				// Static state to the 'reduce' function.
				struct ReduceEnv {
					// Env for recursive calls to reduce.
					ActorEnv old;

					// Production and rule being reduced.
					Nat production;
					Nat rule;

					// All nodes traversed to get here. Updated during the recursion.
					GcArray<StackItem *> *path;
				};

				// Reduce a production of length 'len' from the current stack item. If 'through' is
				// set, only nodes where the edge 'link' is passed are considered.
				void reduce(const ReduceEnv &env, StackItem *stack, StackItem *through, Nat len);

				// Limited reduction of a rule. Only paths passing through the edge 'link' are considered.
				void limitedReduce(const ReduceEnv &env, Set<StackItem *> *top, StackItem *through);

				// Produce error messages from the state set 'states'.
				void errorMsg(StrBuf *out, Nat pos, Set<StackItem *> *states) const;
				void errorMsg(Set<Str *> *errors, Nat state) const;

				/**
				 * Tree computation.
				 */

				// Create the tree node starting at 'node'.
				Node *tree(TreeNode *node, Nat endPos) const;

				// Fill in 'result' from the subtree generated by a pseudo-production.
				void subtree(Node *result, TreeNode *node, Nat endPos) const;

				// Fill in a token.
				void setToken(Node *result, TreeNode *node, Nat endPos, Token *token) const;

				// Create a syntax node for the production 'p'.
				Node *allocNode(Production *p, Nat pos) const;

				/**
				 * Info tree computation.
				 */

				// Create the info node starting at 'node'.
				InfoNode *infoTree(TreeNode *node, Nat endPos) const;

				// Fill in 'result'. from the subtree generated by a pseudo-production.
				void infoSubtree(InfoInternal *result, Nat &resultPos, TreeNode *node, Nat endPos) const;

				// Fill in a token.
				void infoToken(InfoInternal *result, Nat &resultPos, TreeNode *node, Nat endPos, Token *token) const;

				// Compute the number of nodes for 'node' considering pseudo productions for repetitions.
				Nat totalLength(TreeNode *node) const;
			};

		}
	}
}
