#pragma once
#include "Type.h"
#include "Parse.h"
#include "Token.h"
#include "Shared/EnginePtr.h"

namespace storm {
	namespace syntax {

		/**
		 * Iterator for an Option.
		 *
		 * This iterator does not follow the convention in Storm, as options are not
		 * deterministic. This indeterminism is due to the ?,+ and *. However, it is only ever
		 * possible to enter two new states from an existing state, which is why nextA and nextB
		 * suffice. firstA and nextA are just advancing through the linear stream of tokens while
		 * firstB and nextB accounts for any jumps that can occur in the sequence.
		 *
		 * Note: the implementation of OptionIter breaks the thread-safety a bit, as it reads from
		 * the Option from a thread that is not neccessarily the Compiler thread. Should not be too
		 * bad, but is worth noticing.
		 */
		class OptionIter {
			friend class Option;
			STORM_VALUE;
		public:
			// Create an iterator that always is at the end.
			STORM_CTOR OptionIter();

			// Are we at the end of the iteration?
			Bool STORM_FN end() const;

			// Is this a valid iterator?
			Bool STORM_FN valid() const;

			// Get the two iterators which follows this. Returns an iterator where 'end' == true if
			// the operation is not feasible.
			OptionIter STORM_FN nextA() const;
			OptionIter STORM_FN nextB() const;

			// Compare for equality.
			Bool STORM_FN operator ==(const OptionIter &o) const;
			Bool STORM_FN operator !=(const OptionIter &o) const;

			// At start/end of repeat?
			Bool STORM_FN repStart() const;
			Bool STORM_FN repEnd() const;

			// Get the rule this option is a part of (may be null).
			Rule *rulePtr() const;

			// Get the option (borrowed ptr).
			inline Option *optionPtr() const { return o; }

			// Get the current token (borrowed ptr).
			Token *tokenPtr() const;

			// Storm semantics:
			MAYBE(Rule *) STORM_FN rule() const;
			Option *STORM_FN option() const;
			Token *STORM_FN token() const;

		private:
			// Create.
			OptionIter(Par<Option> option, Nat pos = 0);

			// The option we own. We only keep a weak pointer here, as we do not want to incur any
			// overhead when copying or creating one of these objects (this is don *a lot* of times
			// when parsing).
			Option *o;

			// Position into the option's tokens.
			Nat pos;

			// Output.
			friend wostream &operator <<(wostream &to, const OptionIter &o);
		};

		// Output.
		wostream &operator <<(wostream &to, const OptionIter &o);
		Str *STORM_ENGINE_FN toS(EnginePtr e, OptionIter o);

		// Fwd declaration.
		class OptionType;

		/**
		 * Syntax option.
		 */
		class Option : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR Option();
			STORM_CTOR Option(Par<OptionType> owner, Par<OptionDecl> decl, MAYBE(Par<Rule>) delim, Scope scope);

			// Owning rule (may be null). (rulePtr returns borrowed ptr).
			Rule *rulePtr() const;
			MAYBE(Rule *) STORM_FN rule() const;

			// Owning type.
			OptionType *typePtr() const;
			MAYBE(OptionType *) type() const;

			// Tokens.
			STORM_VAR Auto<ArrayP<Token>> tokens;

			// Priority.
			STORM_VAR Int priority;

			// Repeat logic.
			STORM_VAR Nat repStart;
			STORM_VAR Nat repEnd;
			STORM_VAR RepType repType;

			// Capture a raw string between repStart and repEnd?
			STORM_VAR MAYBE(Auto<Token>) repCapture;

			// Is the token at position x inside a repeat?
			Bool STORM_FN inRepeat(Nat pos) const;

			// Get an iterator into this option.
			OptionIter STORM_FN firstA();
			OptionIter STORM_FN firstB();

			// Output which supports adding a marker.
			void output(wostream &to, nat mark, bool bindings = true) const;

		protected:
			virtual void output(wostream &to) const;

		private:
			// Owning OptionType (borrowed ptr).
			OptionType *owner;

			// Output the end of a repetition.
			void outputRepEnd(wostream &to, bool bindings) const;

			// Load tokens from a declaration.
			void loadTokens(Par<OptionDecl> decl, Par<Rule> delim, const Scope &scope);

			// Add a single token to us.
			void addToken(Par<TokenDecl> decl, Par<Rule> delim, const SrcPos &pos, const Scope &scope, Nat &counter);

			// Create a target for a token (if needed).
			TypeVar *createTarget(Par<TokenDecl> decl, Par<Token> token, Nat pos, Nat &counter);

			// Create a target for a token, assuming it is needed.
			TypeVar *createTarget(Value type, Par<TokenDecl> token, Nat &counter);
		};

		/**
		 * The type used to name and find a syntax option.
		 *
		 * This is represented as a type, which inherits from a Rule type. This class will override
		 * the 'eval' member of the parent type.
		 *
		 * These types contain variables representing the syntax trees for the captured parts of the
		 * option. Parts that are captured outside of any repetition are stored as regular
		 * variables, parts that are repeated zero or one time are stored as Maybe<T> and parts that
		 * are repeated zero or more times are stored as Array<T>. Aside from that, the variable
		 * 'pos' is created. It holds the SrcPos of the beginning of the match.
		 *
		 * Note: Types created by this class currently does _not_ have any constructor except a copy
		 * constructor. The parser will instantiate these using a custom (read 'hacky') constructor
		 * to keep it clean and simple. However, we do want to add a constructor that can initialize
		 * all members properly.
		 */
		class OptionType : public Type {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR OptionType(Par<Str> name, Par<OptionDecl> decl, MAYBE(Par<Rule>) delim, Scope scope);

			// Declared at.
			STORM_VAR SrcPos pos;

			// Option.
			STORM_VAR Auto<Option> option;

			// Get owner rule.
			Rule *STORM_FN rule() const;
			Rule *rulePtr() const;

			// Load contents.
			virtual Bool STORM_FN loadAll();

			// All members needing initialization (arrays).
			vector<TypeVar *> arrayMembers;

			// Add members.
			virtual void STORM_FN add(Par<Named> m);

			// Clear.
			virtual void clear();

		protected:
			virtual void output(wostream &to) const;

		private:
			// The declaration (stored only until the first time 'loadAll' is called).
			Auto<OptionDecl> decl;

			// Save the scope for a while.
			Scope scope;
		};


	}
}
