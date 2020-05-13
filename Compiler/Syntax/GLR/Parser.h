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

				// Parse a string, performing error recovery.
				virtual InfoErrors parseApprox(Rule *root, Str *str, Url *file, Str::Iter start, MAYBE(InfoInternal *) ctx);

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

				// Store tree nodes.
				TreeStore *store;

				// Stacks for future steps.
				FutureStacksZ *stacks;

				// Root rule for this parse.
				Nat parseRoot;

				// Start position of the last parse.
				Nat startPos;

				// Source string being parsed.
				Str *source;

				// Url of the source string.
				Url *sourceUrl;

				// Last found stack which accepted the string. Starts with a dummy stack item for
				// the topmost production.
				StackItemZ *acceptingStack;

				// The last non-empty state set. Used for error reporting.
				Set<StackItemZ *> *lastSet;

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
				StackItemZ *startState(Nat pos, Rule *rule);

				// Add 'production' and all other productions which may complete this production to
				// 'alwaysReduce'.
				void findAlwaysReduce(Nat production);

				// Clear all data derived from the syntax.
				void clearSyntax();

				/**
				 * Set-up and tear down of parsing.
				 */

				// Initialize parsing of 'str'.
				void initParse(Rule *root, Str *str, Url *file, Str::Iter start);

				// Remove any temporary state used during parsing.
				void finishParse(MAYBE(InfoInternal *) context);

				// Parse from position 'i' to completion. Assumes 'stacks->top()' contains valid information.
				void doParse(Nat from);

				// Parse as 'doParse' does. Keep track of the last states before the last line
				// ending as well as the error information in 'doParse'. Used for better error recovery.
				void doParse(Nat from, Set<StackItemZ *> *&states, Nat &pos);

				/**
				 * Error recovery specials.
				 */

				// Advance all states on the current stack top.
				void advanceAll();
				void shiftAll();
				void reduceAll();

				// Perform all shifts possible at the current stack top. Returns 'true' if any
				// future states were generated.
				bool actorShift();

				// Perform all possible shifts in the current stack top. Returns 'true' if one or
				// more shifts were performed. Returns 'true' if a matching shift was found.
				bool shiftAll(StackItemZ *now);
				bool shiftAll(StackItemZ *now, Array<Action> *actions);
				void shiftAll(StackItemZ *now, Map<Nat, Nat> *actions);

				/**
				 * Parsing functions. Based on the paper "Parser Generation for Interactive
				 * Environments" by Jan Renkers.
				 */

				// Act on all states until we're done.
				void actor(Nat pos, Set<StackItemZ *> *states);

				// Data passed to the reduction actor.
				struct ActorEnv {
					// Current state.
					State *state;

					// Top of stack before reductions started.
					StackItemZ *stack;

					// Reduce all states, even states that have not reached completion. Used when
					// performing error correction. Contains information on which states have
					// already been reduced.
					bool reduceAll;
				};

				// Perform actions required for a state.
				bool actorShift(const ActorEnv &env);
				void actorReduce(const ActorEnv &env, StackItemZ *through);
				void doReduce(const ActorEnv &env, Nat production, StackItemZ *through);

				// Perform reductions on all states, even if a reduction action is not present.'
				void actorReduceAll(const ActorEnv &env, StackItemZ *through);

				// Static state to the 'reduce' function.
				struct ReduceEnv {
					// Env for recursive calls to reduce.
					const ActorEnv &old;

					// Production and rule being reduced.
					Nat production;
					Nat rule;

					// Number of items of the currently reduced production.
					Nat length;

					// Shift-errors to be added to the current production.
					Nat errors;
				};

				// Linked list of entries, keeping track of the path currently being reduced.
				struct Path {
					const Path *prev;
					StackItemZ *item;
				};

				// Reduce a production of length 'len' from the current stack item. If 'through' is
				// set, only nodes where the edge 'link' is passed are considered.
				void reduce(const ReduceEnv &env, StackItemZ *stack, const Path *path, StackItemZ *through, Nat len);
				void finishReduce(const ReduceEnv &env, StackItemZ *stack, const Path *path);

				// Limited reduction of a rule. Only paths passing through the edge 'link' are considered.
				void limitedReduce(const ReduceEnv &env, Set<StackItemZ *> *top, StackItemZ *through);

				// Produce error messages from the state set 'states'.
				void errorMsg(StrBuf *out, Nat pos, Set<StackItemZ *> *states) const;
				void errorMsg(Set<Str *> *errors, Nat state) const;

				// Produce error messages related to an unfullfilled requirement in the stack items provided.
				void reqErrorMsg(StrBuf *out, StackItemZ *states) const;
				Nat findMissingReq(Nat tree, ParentReq required) const;

				/**
				 * Tree computation.
				 */

				// Create the tree node starting at 'node'.
				Node *tree(const TreeNode &node, Nat endPos) const;

				// Fill in 'result' from the subtree generated by a pseudo-production.
				void subtree(Node *result, TreeNode &node, Nat endPos) const;

				// Fill in a token.
				void setToken(Node *result, TreeNode &node, Nat endPos, Token *token) const;

				// Create a syntax node for the production 'p'.
				Node *allocNode(Production *p, Nat start, Nat end) const;

				/**
				 * Info tree computation.
				 */

				// Create the info node starting at 'node'.
				InfoNode *infoTree(const TreeNode &node, Nat endPos) const;

				// Fill in 'result'. from the subtree generated by a pseudo-production. Returns # of errors.
				InfoErrors infoSubtree(InfoInternal *result, Nat &resultPos, TreeNode &node, Nat endPos) const;

				// Fill in a token. Returns # of errors.
				InfoErrors infoToken(InfoInternal *result, Nat &resultPos, TreeNode &node, Nat endPos, Token *token) const;

				// Compute the number of nodes for 'node' considering pseudo productions for repetitions.
				Nat totalLength(const TreeNode &node) const;

				/**
				 * Full info tree computation.
				 */

				// Construct completion nodes for the last, uncompleted states.
				void completePrefix();
			};

		}
	}
}
