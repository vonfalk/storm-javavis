#include "stdafx.h"
#include "Production.h"
#include "Compiler/Lib/Array.h"
#include "Compiler/Lib/Maybe.h"
#include "Compiler/TypeCtor.h"
#include "Utils/Memory.h"
#include "Exception.h"
#include "Node.h"
#include "SStr.h"
#include "Rule.h"
#include "Transform.h"

namespace storm {
	namespace syntax {

		Production::Production() {
			tokens = new (this) Array<Token *>();
		}

		Production::Production(ProductionType *owner, ProductionDecl *decl, MAYBE(Rule *) delim, Scope scope) {
			this->owner = owner;
			tokens = new (this) Array<Token *>();
			priority = decl->priority;
			repStart = decl->repStart;
			repEnd = decl->repEnd;
			repType = decl->repType;

			Nat counter = 0;
			if (decl->repCapture) {
				MemberVar *r = createTarget(Value(SStr::stormType(engine())), decl->repCapture, counter);
				if (r) {
					owner->add(r);
					repCapture = new (this) Token();
					repCapture->target = r;
					repCapture->invoke = decl->repCapture->invoke;
					repCapture->raw = decl->repCapture->raw;
				}
			}

			for (nat i = 0; i < decl->tokens->count(); i++) {
				addToken(decl->tokens->at(i), delim, decl->pos, scope, counter);
			}
		}

		void Production::addToken(TokenDecl *decl, MAYBE(Rule *) delim, SrcPos pos, Scope scope, Nat &counter) {
			Token *token = null;
			if (RegexTokenDecl *r = as<RegexTokenDecl>(decl)) {
				token = new (this) RegexToken(r->regex);
			} else if (RuleTokenDecl *u = as<RuleTokenDecl>(decl)) {
				if (Rule *rule = as<Rule>(scope.find(u->rule))) {
					token = new (this) RuleToken(rule);
				} else {
					throw SyntaxError(pos, L"The rule " + ::toS(u->rule) + L" does not exist.");
				}
			} else if (DelimTokenDecl *d = as<DelimTokenDecl>(decl)) {
				if (delim) {
					token = new (this) DelimToken(delim);
				} else {
					throw SyntaxError(pos, L"No delimiter was declared in this file.");
				}
			} else {
				throw InternalError(L"Unknown subtype of TokenDecl found: " + ::toS(decl));
			}

			if (decl->store != null || decl->invoke != null) {
				MemberVar *r = createTarget(decl, token, tokens->count(), counter);
				if (r) {
					owner->add(r);
					token->target = r;
					token->invoke = decl->invoke;
					token->raw = decl->raw;
				}
			}

			token->color = decl->color;
			tokens->push(token);
		}

		MAYBE(MemberVar *) Production::createTarget(TokenDecl *decl, Token *token, Nat pos, Nat &counter) {
			Value type;
			if (as<RegexToken>(token)) {
				type = Value(SStr::stormType(engine()));
			} else if (RuleToken *rule = as<RuleToken>(token)) {
				type = Value(rule->rule);
			} else {
				throw InternalError(L"Unknown subtype of Token found: " + ::toS(token));
			}

			if (inRepeat(pos)) {
				if (repType == repZeroOne) {
					type = wrapMaybe(type);
				} else {
					type = wrapArray(type);
				}
			}

			return createTarget(type, decl, counter);
		}

		MAYBE(MemberVar *) Production::createTarget(Value type, TokenDecl *decl, Nat &counter) {
			if (decl->store) {
				return new (this) MemberVar(decl->store, type, owner);
			} else if (decl->invoke) {
				StrBuf *name = new (this) StrBuf();
				*name << decl->invoke << (counter++);
				return new (this) MemberVar(name->toS(), type, owner);
			} else {
				return null;
			}
		}

		Bool Production::inRepeat(Nat token) const {
			return repType != repNone
				&& repStart <= token
				&& repEnd > token;
		}

		Rule *Production::rule() const {
			if (owner)
				return owner->rule();
			else
				return null;
		}

		ProductionType *Production::type() const {
			return owner;
		}

		void Production::toS(StrBuf *to) const {
			toS(to, tokens->count() + 1);
		}

		void Production::toS(StrBuf *to, Nat mark) const {
			toS(to, mark, true);
		}

		void Production::toS(StrBuf *to, Nat mark, Bool bindings) const {
			if (Rule *r = rule())
				*to << r->identifier();
			else
				*to << L"?";

			if (priority != 0)
				*to << L"[" << priority << L"]";

			*to << L" : ";

			bool usingRep = repStart < repEnd && repType != repNone;
			bool prevDelim = false;
			for (nat i = 0; i < tokens->count(); i++) {
				Token *token = tokens->at(i);
				bool currentDelim = as<DelimToken>(token) != null;

				if (usingRep && repEnd == i)
					outputRepEnd(to, bindings);

				if (i > 0 && !currentDelim && !prevDelim)
					*to << L" - ";

				if (usingRep && repStart == i)
					*to << L"(";

				if (i == mark)
					*to << L"<>";

				if (currentDelim)
					*to << L", ";
				else
					token->toS(to, bindings);

				prevDelim = currentDelim;
			}

			if (usingRep && repEnd == tokens->count())
				outputRepEnd(to, bindings);

			if (mark == tokens->count())
				*to << L"<>";

			if (owner)
				*to << L" = " << owner->name;
		}

		void Production::outputRepEnd(StrBuf *to, Bool bindings) const {
			*to << L")";

			if (repType == repZeroOne) {
				*to << L"?";
			} else if (repType == repOnePlus) {
				*to << L"+";
			} else if (repType == repZeroPlus) {
				*to << L"*";
			} else if (repCapture) {
				repCapture->toS(to, bindings);
			}
		}

		/**
		 * Iterator.
		 */

		ProductionIter Production::firstA() {
			return ProductionIter(this, 0);
		}

		ProductionIter Production::firstB() {
			if (repStart == 0 && skippable(repType))
				return ProductionIter(this, repEnd);
			else
				return ProductionIter();
		}

		ProductionIter Production::posIter(Nat pos) {
			return ProductionIter(this, pos);
		}

		ProductionIter ProductionIter::nextA() const {
			return ProductionIter(p, pos + 1);
		}

		ProductionIter ProductionIter::nextB() const {
			Nat next = pos + 1;
			if (next == p->repStart) {
				// The start of something skippable -> skip it!
				if (skippable(p->repType))
					return ProductionIter(p, p->repEnd);
			} else if (next == p->repEnd) {
				// The end of a repetition -> repeat!
				if (repeatable(p->repType))
					return ProductionIter(p, p->repStart);
			}

			// Nothing possible, just return something invalid.
			return ProductionIter();
		}

		ProductionIter::ProductionIter() : p(null), pos(0) {}

		ProductionIter::ProductionIter(Production *p, Nat pos) : p(p), pos(pos) {}

		Bool ProductionIter::end() const {
			return p != null
				&& pos >= p->tokens->count();
		}

		Bool ProductionIter::valid() const {
			return p != null;
		}

		Bool ProductionIter::operator ==(const ProductionIter &o) const {
			return p == o.p
				&& pos == o.pos;
			// We ignore repCount and lie a little bit. This prevents infinite repetitions of zero-rules.
		}

		Bool ProductionIter::operator !=(const ProductionIter &o) const {
			return !(*this == o);
		}

		Bool ProductionIter::repStart() const {
			return pos == p->repStart;
		}

		Bool ProductionIter::repEnd() const {
			return pos == p->repEnd;
		}

		MAYBE(Rule *) ProductionIter::rule() const {
			if (p)
				return p->rule();
			else
				return null;
		}

		MAYBE(Production *) ProductionIter::production() const {
			return p;
		}

		MAYBE(Token *) ProductionIter::token() const {
			if (!end())
				return p->tokens->at(pos);
			else
				return null;
		}

		wostream &operator <<(wostream &to, const ProductionIter &i) {
			if (!i.valid())
				return to << L"<invalid>";
			StrBuf *b = new (i.production()) StrBuf();
			*b << i;
			return to << b->toS()->c_str();
		}

		StrBuf &operator <<(StrBuf &to, ProductionIter i) {
			if (!i.valid())
				to << L"<invalid>";
			else
				i.p->toS(&to, i.pos, false);
			return to;
		}


		/**
		 * Type
		 */

		ProductionType::ProductionType(Str *name, ProductionDecl *decl, MAYBE(Rule *) delim, Scope scope)
			: Type(name, typeClass),
			  pos(decl->pos),
			  decl(decl),
			  scope(scope) {

			Rule *r = as<Rule>(scope.find(decl->rule));
			if (!r)
				throw SyntaxError(pos, L"The rule " + ::toS(decl->rule) + L" was not found.");
			setSuper(r);

			arrayMembers = new (this) Array<MemberVar *>();

			// Can not be created in the initializer list as it accesses us in 'add'.
			production = new (this) Production(this, decl, delim, scope);
		}

		void ProductionType::add(Named *m) {
			Type::add(m);

			if (MemberVar *v = as<MemberVar>(m)) {
				// Only arrays are interesting.
				if (isArray(v->type))
					arrayMembers->push(v);
			}
		}

		Rule *ProductionType::rule() const {
			Rule *r = as<Rule>(super());
			if (!r)
				throw InternalError(L"An option does not inherit from Rule!");
			return r;
		}

		void ProductionType::toS(StrBuf *to) const {
			*to << production << L"\n";
			Type::toS(to);
		}

		static void outputVar(Node *me, StrBuf *to, MemberVar *var) {
			if (!var)
				return;

			*to << var->name << L" : ";
			int offset = var->offset().current();
			if (isArray(var->type)) {
				Array<Object *> *v = OFFSET_IN(me, offset, Array<Object *> *);
				if (v)
					v->toS(to);
				else
					*to << L"null";
			} else {
				// It is a TObject or Object. Always on the same thread as us.
				TObject *v = OFFSET_IN(me, offset, TObject *);
				if (v)
					v->toS(to);
				else
					*to << L"null";
			}
			*to << L"\n";
		}

		// This is implemented in C++ since it does not really need to be fast, and would be painful
		// to implement in machine code.
		void CODECALL productionToS(Node *me, StrBuf *to) {
			// We know this is true, otherwise we would not be here...
			ProductionType *type = (ProductionType *)runtime::typeOf(me);
			Production *prod = type->production;

			*to << L"{\n";
			{
				Indent z(to);

				if (Rule *r = prod->rule())
					*to << r->identifier() << L" -> ";
				*to << type->identifier() << L"\n";

				for (nat i = 0; i < prod->tokens->count(); i++) {
					Token *t = prod->tokens->at(i);
					outputVar(me, to, t->target);
				}

				if (prod->repCapture)
					outputVar(me, to, prod->repCapture->target);
			}
			*to << L"}";
		}

		Bool ProductionType::loadAll() {
			assert(decl, L"loadAll called twice!");

			Engine &e = engine;
			Value me = thisPtr(this);
			Value strBuf = thisPtr(StrBuf::stormType(e));

			add(createTransformFn(decl, this, scope));

			Array<Value> *p = new (e) Array<Value>(2, me);
			p->at(1) = strBuf;
			add(nativeFunction(e, Value(), L"toS", p, &productionToS));

			if (!me.isActor()) {
				add(new (e) TypeCopyCtor(this));
				add(new (e) TypeDeepCopy(this));
			}

			// Clear!
			decl = null;

			return Type::loadAll();
		}

	}
}
