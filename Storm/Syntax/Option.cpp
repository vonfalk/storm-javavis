#include "stdafx.h"
#include "Option.h"
#include "Rule.h"
#include "TypeCtor.h"
#include "TypeDtor.h"
#include "Exception.h"

namespace storm {
	namespace syntax {

		Option::Option(Par<Str> name, Par<OptionDecl> decl, Par<Rule> delim, Scope scope) :
			Type(name->v, typeClass),
			pos(pos),
			scope(scope),
			tokens(CREATE(ArrayP<Token>, this)),
			priority(0),
			repStart(0),
			repEnd(0) {

			Auto<Named> found = scope.find(decl->rule);
			if (!found)
				throw SyntaxError(pos, L"The rule " + ::toS(decl->rule) + L" was not found!");
			Auto<Rule> r = found.as<Rule>();
			if (!r)
				throw SyntaxError(pos, ::toS(decl->rule) + L" is not a rule.");
			setSuper(r);

			loadTokens(decl, delim);
		}

		Rule *Option::rule() const {
			Rule *r = as<Rule>(super());
			assert(r, L"An option does not inherit from Rule!");
			return r;
		}

		void Option::loadTokens(Par<OptionDecl> decl, Par<Rule> delim) {
			priority = decl->priority;
			repStart = decl->repStart;
			repEnd = decl->repEnd;
			repType = decl->repType;

			for (Nat i = 0; i < decl->tokens->count(); i++) {
				addToken(decl->tokens->at(i), delim, decl->pos);
			}
		}

		void Option::addToken(Par<TokenDecl> decl, Par<Rule> delim, const SrcPos &optionPos) {
			Auto<Token> token;

			if (RegexTokenDecl *r = as<RegexTokenDecl>(decl.borrow())) {
				token = CREATE(RegexToken, this, r->regex);
			} else if (RuleTokenDecl *u = as<RuleTokenDecl>(decl.borrow())) {
				Auto<Named> found = scope.find(u->rule);
				if (Rule *rule = as<Rule>(found.borrow())) {
					token = CREATE(RuleToken, this, rule);
				} else {
					throw SyntaxError(u->pos, L"The rule " + ::toS(u->rule) + L" does not exist.");
				}
			} else if (DelimTokenDecl *d = as<DelimTokenDecl>(decl.borrow())) {
				if (delim) {
					token = CREATE(DelimToken, this, delim);
				} else {
					throw SyntaxError(optionPos, L"No delimiter was declare in this file.");
				}
			} else {
				throw InternalError(L"Unknown sub-type to TokenDecl found: " + ::toS(decl));
			}

			tokens->push(token);
		}

		bool Option::loadAll() {
			// Load our functions!

			// Add these last.
			add(steal(CREATE(TypeDefaultCtor, this, this)));
			add(steal(CREATE(TypeCopyCtor, this, this)));
			add(steal(CREATE(TypeDeepCopy, this, this)));
			add(steal(CREATE(TypeDefaultDtor, this, this)));

			return Type::loadAll();
		}

		void Option::output(wostream &to) const {
			to << rule()->identifier();

			if (priority != 0)
				to << '[' << priority << ']';

			to << L" : ";

			bool usingRep = repStart < repEnd;
			bool prevDelim = false;
			for (Nat i = 0; i < tokens->count(); i++) {
				const Auto<Token> &token = tokens->at(i);
				bool currentDelim = as<DelimToken>(token.borrow()) != null;

				if (usingRep && repEnd == i)
					outputRepEnd(to);

				if (i > 0 && !currentDelim && !prevDelim)
					to << L" - ";

				if (usingRep && repStart == i)
					to << L"(";

				if (currentDelim) {
					to << L", ";
				} else {
					to << token;
				}

				prevDelim = currentDelim;
			}

			if (usingRep && repEnd == tokens->count())
				outputRepEnd(to);

			to << L" = " << name << L";";
		}

		void Option::outputRepEnd(wostream &to) const {
			to << ')';

			if (repType == repZeroOne()) {
				to << '?';
			} else if (repType == repOnePlus()) {
				to << '+';
			} else if (repType == repZeroPlus()) {
				to << '*';
			}
		}



		/**
		 * OptionIter.
		 */

		OptionIter Option::firstA() {
			return OptionIter(this);
		}

		OptionIter Option::firstB() {
			if (repStart == 0 && repType.skippable()) {
				return OptionIter(this, repEnd);
			}

			return OptionIter(null);
		}

		OptionIter OptionIter::nextA() const {
			return OptionIter(option, pos + 1);
		}

		OptionIter OptionIter::nextB() const {
			Nat next = pos + 1;

			if (next == option->repStart) {
				// If we're at the start of something skippable, skip it.
				if (option->repType.skippable()) {
					return OptionIter(option, option->repEnd);
				}
			} else if (next == option->repEnd) {
				// End of the repetition. Shall we repeat?
				if (option->repType.repeatable()) {
					return OptionIter(option, option->repStart);
				}
			}

			// Nothing possible, just return an invalid result.
			return OptionIter(null);
		}

		OptionIter::OptionIter(Par<Option> o, Nat pos) : option(o.borrow()), pos(pos) {}

		bool OptionIter::end() const {
			return option == null
				|| pos >= option->tokens->count();
		}

		bool OptionIter::operator ==(const OptionIter &o) const {
			return option == o.option
				&& pos == o.pos;
			// We ignore repCount and lie a little bit. This prevents infinite repetitions of zero-rules.
		}

		bool OptionIter::operator !=(const OptionIter &o) const {
			return !(*this == o);
		}

		wostream &operator <<(wostream &to, const OptionIter &o) {
			to << L"TODO";
			return to;
		}

		Str *toS(EnginePtr e, OptionIter o) {
			return CREATE(Str, e.v, ::toS(o));
		}

	}
}
