#pragma once
#include "Decl.h"
#include "Token.h"
#include "Delimiters.h"
#include "Compiler/Type.h"
#include "Compiler/Scope.h"

namespace storm {
	namespace syntax {
		STORM_PKG(lang.bnf);

		class Production;
		class ProductionType;

		/**
		 * Iterator for a production.
		 *
		 * This iterator does not follow the convention in Storm since productions are not
		 * deterministic. This is due to the ?, + and * operators. However, it is only ever possible
		 * to enter two new states from an existing state, so it is sufficient to inspect two
		 * possible 'next' functions. There are a number of pairs with different properties, as follows:
		 *
		 * - first{A,B}, next{A,B}:
		 *   These are the simplest form. {first,next}A always traverses the linear stream of tokens
		 *   and {first,next}B accounts for any jumps that may occur.
		 * - first{Long,Short}, next{Long,Short}:
		 *   The {first,next}Long always gives an iterator with the longest potential match (i.e. jumps
		 *   backwards if possible), while {first,next}Short gives the shortest potential match
		 *   (i.e. jumps forwards if possible).
		 * - firstFixed, nextFixed:
		 *   These functions traverse the iterator so that a fixed number of repetitions are done
		 *   in total, regardless of whether or not this is allowed by the repetition mode. This is
		 *   convenient while a parser traverses a previous match and attempts to map that back to
		 *   the grammar. This pair of functions stores some state in an external variable to keep
		 *   track of the number of iterations.
		 */
		class ProductionIter {
			STORM_VALUE;
			friend class Production;
		public:
			// Create an invalid iterator.
			STORM_CTOR ProductionIter();

			// Are we at the end of the iteration?
			Bool STORM_FN end() const;

			// Is this a valid iterator?
			Bool STORM_FN valid() const;

			// Get the two iterators which follow this. Returns an iterator where 'valid' == false
			// if the operation is not feasible.
			ProductionIter STORM_FN nextA() const;
			ProductionIter STORM_FN nextB() const;

			// Long/short matches.
			ProductionIter STORM_FN nextLong() const;
			ProductionIter STORM_FN nextShort() const;

			// A particular number of iterations.
			ProductionIter STORM_FN nextFixed(Nat &n) const;

			// Compare for equality.
			Bool STORM_FN operator ==(const ProductionIter &o) const;
			Bool STORM_FN operator !=(const ProductionIter &o) const;

			// At start/end of repeat?
			Bool STORM_FN repStart() const;
			Bool STORM_FN repEnd() const;

			// Get the position in the production.
			inline Nat STORM_FN position() const { return pos; }

			// Get the rule this option is a part of.
			MAYBE(Rule *) rule() const;

			// Get the production we're a part of.
			MAYBE(Production *) production() const;

			// Get the current token.
			MAYBE(Token *) token() const;

		private:
			// Create.
			ProductionIter(Production *p, Nat pos);

			// The production we refer to.
			Production *p;

			// Position into the option's tokens.
			Nat pos;

			// Output.
			friend StrBuf &operator <<(StrBuf &to, ProductionIter i);
		};

		wostream &operator <<(wostream &to, const ProductionIter &o);
		StrBuf &STORM_FN operator <<(StrBuf &to, ProductionIter i);


		/**
		 * Syntax production.
		 */
		class Production : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create with no parent.
			STORM_CTOR Production();

			// Create, but populate manually.
			STORM_CTOR Production(ProductionType *owner);

			// Create.
			STORM_CTOR Production(ProductionType *owner, ProductionDecl *decl, Delimiters *delim, Scope scope);

			// Owning rule.
			MAYBE(Rule *) STORM_FN rule() const;

			// Owning type.
			MAYBE(ProductionType *) STORM_FN type() const;

			// Parent syntax element. When parsing, this element needs to be an indirect parent of
			// this production. Otherwise this production should not be considered.
			MAYBE(Rule *) parent;

			// Tokens.
			Array<Token *> *tokens;

			// Priority.
			Int priority;

			// Repeat logic.
			Nat repStart;
			Nat repEnd;
			RepType repType;

			// Indentation logic.
			Nat indentStart;
			Nat indentEnd;
			IndentType indentType;

			// Capture a raw string between repStart and repEnd?
			MAYBE(Token *) repCapture;

			// Is the position 'n' inside a repeat?
			Bool STORM_FN inRepeat(Nat pos) const;

			// Create iterators.
			ProductionIter STORM_FN firstA();
			ProductionIter STORM_FN firstB();

			// Long/short versions.
			ProductionIter STORM_FN firstLong();
			ProductionIter STORM_FN firstShort();

			// Fixed iterations.
			ProductionIter STORM_FN firstFixed(Nat &n);

			// Compute iterations required to traverse 'n' tokens in total.
			Nat STORM_FN repetitionsFor(Nat n);

			// Create an iterator at a specific position.
			ProductionIter posIter(Nat pos);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;
			void STORM_FN toS(StrBuf *to, Nat mark) const;
			void STORM_FN toS(StrBuf *to, Nat mark, Bool bindings) const;

		private:
			// Owner.
			ProductionType *owner;

			// Output the end of a repeat.
			void outputRepEnd(StrBuf *to, Bool bindings) const;

			// Add a single token here.
			void addToken(TokenDecl *decl, Delimiters *delim, SrcPos pos, Scope scope, Nat &counter);

			// Create a target for a token (if needed).
			MAYBE(MemberVar *) createTarget(SrcPos pos, Value type, TokenDecl *token, Nat &counter);
			MAYBE(MemberVar *) createTarget(SrcPos p, TokenDecl *decl, Token *token, Nat pos, Nat &counter);
		};

		/**
		 * The type used to name and find a syntax option.
		 *
		 * This is represented as a type which inherits from a Rule type. This class will override
		 * the 'transform' of the parent type.
		 *
		 * These types also contain variables representing the syntax trees for the captured parts
		 * of the production. Tokens captured outside of any repetition are stored as regular
		 * variables, tokens repeated zero or one time are stored as Maybe<T> and parts that are
		 * repeated zero or more times are stored as Array<T>.
		 *
		 * Note: Types created by this does not currently have any constructor except the copy
		 * constructor. The parser will create these using a custom (read hacky) constructor to keep
		 * it clean and simple. However, we do want to add a constructor that can initialize all
		 * members properly.
		 */
		class ProductionType : public Type {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR ProductionType(Str *name, ProductionDecl *decl, Delimiters *delim, Scope scope);

			// Create, populate manually later.
			STORM_CTOR ProductionType(SrcPos pos, Str *name, Rule *rule);

			// Declared at.
			SrcPos pos;

			// Production.
			Production *production;

			// Get the owner rule.
			Rule *STORM_FN rule() const;

			// All members needing initialization (ie. arrays).
			Array<MemberVar *> *arrayMembers;

			// Add members.
			virtual void STORM_FN add(Named *m);

			// Output.
			virtual void STORM_FN toS(StrBuf *to) const;

		protected:
			// Load contents.
			virtual Bool STORM_FN loadAll();

		private:
			// The declaration, stored only until the first time 'loadAll' is called.
			MAYBE(ProductionDecl *) decl;

			// Save the scope for a while.
			Scope scope;
		};


		// For inlining:
		inline Bool ProductionIter::end() const {
			return p != null
				&& pos >= p->tokens->count();
		}

		// For inlining:
		inline Bool ProductionIter::valid() const {
			return p != null;
		}

		// For inlining:
		inline MAYBE(Token *) ProductionIter::token() const {
			if (!end())
				return p->tokens->at(pos);
			else
				return null;
		}


	}
}
