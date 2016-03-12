#include "stdafx.h"
#include "Option.h"
#include "OptionTfm.h"
#include "Rule.h"
#include "TypeCtor.h"
#include "TypeDtor.h"
#include "Exception.h"
#include "Lib/Maybe.h"
#include "Lib/ArrayTemplate.h"

namespace storm {
	namespace syntax {

		Option::Option() :
			tokens(CREATE(ArrayP<Token>, this)),
			priority(0),
			repStart(0),
			repEnd(0) {}

		Option::Option(Par<OptionType> owner, Par<OptionDecl> decl, Par<Rule> delim, Scope scope) :
			tokens(CREATE(ArrayP<Token>, this)),
			priority(0),
			repStart(0),
			repEnd(0),
			owner(owner.borrow()) {

			loadTokens(decl, delim, scope);
		}

		void Option::loadTokens(Par<OptionDecl> decl, Par<Rule> delim, const Scope &scope) {
			priority = decl->priority;
			repStart = decl->repStart;
			repEnd = decl->repEnd;
			repType = decl->repType;

			Nat counter = 0;

			if (decl->repCapture) {
				Auto<TypeVar> r = createTarget(SStr::stormType(engine()), decl->repCapture, counter);
				if (r) {
					owner->add(r);
					repCapture = CREATE(Token, this);
					repCapture->target = r.borrow();
					repCapture->invoke = decl->repCapture->invoke;
					repCapture->raw = decl->repCapture->raw;
				}
			}

			for (Nat i = 0; i < decl->tokens->count(); i++) {
				addToken(decl->tokens->at(i), delim, decl->pos, scope, counter);
			}
		}

		void Option::addToken(Par<TokenDecl> decl, Par<Rule> delim, const SrcPos &pos, const Scope &scope, Nat &counter) {
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
					throw SyntaxError(pos, L"No delimiter was declare in this file.");
				}
			} else {
				throw InternalError(L"Unknown sub-type to TokenDecl found: " + ::toS(decl));
			}

			if (decl->store != null || decl->invoke != null) {
				Auto<TypeVar> r = createTarget(decl, token, tokens->count(), counter);
				if (r) {
					owner->add(r);
					token->target = r.borrow();
					token->invoke = decl->invoke;
					token->raw = decl->raw;
				}
			}

			tokens->push(token);
		}

		TypeVar *Option::createTarget(Par<TokenDecl> decl, Par<Token> token, Nat pos, Nat &counter) {
			Auto<TypeVar> r;
			Value type;

			if (as<RegexToken>(token.borrow())) {
				type = Value(SStr::stormType(engine()));
			} else if (RuleToken *rule = as<RuleToken>(token.borrow())) {
				type = Value(rule->rule);
			} else {
				assert(false);
			}

			if (inRepeat(pos)) {
				if (repType == repZeroOne()) {
					type = wrapMaybe(type);
				} else {
					type = wrapArray(type);
				}
			}

			return createTarget(type, decl, counter);
		}

		TypeVar *Option::createTarget(Value type, Par<TokenDecl> decl, Nat &counter) {
			if (decl->store) {
				return CREATE(TypeVar, this, owner, type, decl->store->v);
			} else if (decl->invoke) {
				String name = decl->invoke->v + ::toS(counter++);
				return CREATE(TypeVar, this, owner, type, name);
			} else {
				return null;
			}
		}

		Bool Option::inRepeat(Nat token) const {
			return repType != repNone()
				&& repStart <= token
				&& repEnd > token;
		}

		Rule *Option::rulePtr() const {
			if (!owner)
				return null;
			return owner->rulePtr();
		}

		Rule *Option::rule() const {
			Rule *r = rulePtr();
			if (r)
				r->addRef();
			return r;
		}

		OptionType *Option::typePtr() const {
			return owner;
		}

		OptionType *Option::type() const {
			if (owner)
				owner->addRef();
			return owner;
		}

		void Option::output(wostream &to) const {
			output(to, tokens->count() + 1);
		}

		void Option::output(wostream &to, nat mark, bool bindings) const {
			if (Rule *r = rulePtr()) {
				to << r->identifier() << ' ';
			}

			to << L":";
			if (priority != 0)
				to << '[' << priority << ']';
			to << ' ';

			bool usingRep = repStart < repEnd;
			bool prevDelim = false;
			for (Nat i = 0; i < tokens->count(); i++) {
				const Auto<Token> &token = tokens->at(i);
				bool currentDelim = as<DelimToken>(token.borrow()) != null;

				if (usingRep && repEnd == i)
					outputRepEnd(to, bindings);

				if (i > 0 && !currentDelim && !prevDelim)
					to << L" - ";

				if (usingRep && repStart == i)
					to << L"(";

				if (i == mark)
					to << L"<>";

				if (currentDelim) {
					to << L", ";
				} else {
					token->output(to, bindings);
				}

				prevDelim = currentDelim;
			}

			if (usingRep && repEnd == tokens->count())
				outputRepEnd(to, bindings);

			if (mark == tokens->count())
				to << L"<>";

			if (owner) {
				to << L" = " << owner->name;
			}
		}

		void Option::outputRepEnd(wostream &to, bool bindings) const {
			to << ')';

			if (repType == repZeroOne()) {
				to << '?';
			} else if (repType == repOnePlus()) {
				to << '+';
			} else if (repType == repZeroPlus()) {
				to << '*';
			} else if (repCapture) {
				repCapture->output(to, bindings);
			}
		}


		/**
		 * OptionType.
		 */

		OptionType::OptionType(Par<Str> name, Par<OptionDecl> decl, Par<Rule> delim, Scope scope) :
			Type(name->v, typeClass),
			pos(decl->pos),
			decl(decl),
			scope(scope) {

			Auto<Named> found = scope.find(decl->rule);
			if (!found)
				throw SyntaxError(pos, L"The rule " + ::toS(decl->rule) + L" was not found!");
			Auto<Rule> r = found.as<Rule>();
			if (!r)
				throw SyntaxError(pos, ::toS(decl->rule) + L" is not a rule.");
			setSuper(r);

			// Can not be created in initializer list as it accesses the vector in 'add'.
			option = CREATE(Option, this, this, decl, delim, scope);
		}

		Rule *OptionType::rulePtr() const {
			Rule *r = as<Rule>(super());
			assert(r, L"An option does not inherit from Rule!");
			return r;
		}

		Rule *OptionType::rule() const {
			Rule *r = rulePtr();
			r->addRef();
			return r;
		}

		// This is implemented in C++ since it does not really need to be fast, and would be painful
		// to implement in machine code.
		// Note: This may *not* work under x64 due to incompatible calling convention (thiscall vs cdecl).
		Str *CODECALL optionToS(Object *me) {
			std::wostringstream to;

			// We know this is true, otherwise we would not be here...
			OptionType *type = (OptionType *)me->myType;
			Option *option = type->option.borrow();

			typedef map<String, TypeVar *> Data;
			Data data;

			for (Nat i = 0; i < option->tokens->count(); i++) {
				Token *t = option->tokens->at(i).borrow();
				if (t->target)
					data.insert(make_pair(t->target->name, t->target));
			}

			if (option->repCapture && option->repCapture->target) {
				TypeVar *var = option->repCapture->target;
				data.insert(make_pair(var->name, var));
			}

			to << L"{\n";
			{
				Indent z(to);
				to << type->identifier() << endl;

				for (Data::iterator i = data.begin(), end = data.end(); i != end; ++i) {
					to << i->first << L" : ";
					TypeVar *var = i->second;
					Offset offset = var->offset();
					// We can be fairly sure all of them are derived from Object.
					Object *obj = OFFSET_IN(me, offset.current(), Object *);
					if (obj) {
						to << *obj;
					} else {
						to << L"null";
					}
					to << endl;
				}
			}

			to << L"}";

			return CREATE(Str, me, to.str());
		}

		bool OptionType::loadAll() {
			assert(decl, L"loadAll called twice!");

			// Load our functions!
			Engine &e = engine;
			Value me = Value::thisPtr(this);
			Value str = Value::thisPtr(Str::stormType(this));

			add(steal(createTransformFn(decl, this, scope)));
			add(steal(nativeFunction(e, str, L"toS", valList(1, me), &optionToS)));

			// Add these last.
			add(steal(CREATE(TypeDefaultCtor, this, this)));
			add(steal(CREATE(TypeCopyCtor, this, this)));
			add(steal(CREATE(TypeDeepCopy, this, this)));
			add(steal(CREATE(TypeDefaultDtor, this, this)));

			// No need of remembering 'decl' anymore!
			decl = null;

			return Type::loadAll();
		}

		void OptionType::clear() {
			decl = null;
			scope = Scope();
			Type::clear();
		}

		void OptionType::add(Par<Named> named) {
			Type::add(named);

			if (TypeVar *v = as<TypeVar>(named.borrow())) {
				// Only arrays are interesting.
				if (isArray(v->varType))
					arrayMembers.push_back(v);
			}
		}

		void OptionType::output(wostream &to) const {
			to << option << endl;
			Type::output(to);
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
			return OptionIter(o, pos + 1);
		}

		OptionIter OptionIter::nextB() const {
			Nat next = pos + 1;

			if (next == o->repStart) {
				// If we're at the start of something skippable, skip it.
				if (o->repType.skippable()) {
					return OptionIter(o, o->repEnd);
				}
			} else if (next == o->repEnd) {
				// End of the repetition. Shall we repeat?
				if (o->repType.repeatable()) {
					return OptionIter(o, o->repStart);
				}
			}

			// Nothing possible, just return an invalid result.
			return OptionIter(null);
		}

		OptionIter::OptionIter() : o(null), pos(0) {}

		OptionIter::OptionIter(Par<Option> o, Nat pos) : o(o.borrow()), pos(pos) {}

		bool OptionIter::end() const {
			return o == null
				|| pos >= o->tokens->count();
		}

		bool OptionIter::valid() const {
			return o != null;
		}

		bool OptionIter::operator ==(const OptionIter &other) const {
			return o == other.o
				&& pos == other.pos;
			// We ignore repCount and lie a little bit. This prevents infinite repetitions of zero-rules.
		}

		bool OptionIter::operator !=(const OptionIter &o) const {
			return !(*this == o);
		}

		bool OptionIter::repStart() const {
			return pos == o->repStart;
		}

		bool OptionIter::repEnd() const {
			return pos == o->repEnd;
		}

		Rule *OptionIter::rulePtr() const {
			return o->rulePtr();
		}

		Token *OptionIter::tokenPtr() const {
			return o->tokens->at(pos).borrow();
		}

		Rule *OptionIter::rule() const {
			Rule *r = rulePtr();
			if (r)
				r->addRef();
			return r;
		}

		Option *OptionIter::option() const {
			Option *r = optionPtr();
			r->addRef();
			return r;
		}

		Token *OptionIter::token() const {
			Token *r = tokenPtr();
			r->addRef();
			return r;
		}

		wostream &operator <<(wostream &to, const OptionIter &o) {
			if (o.o) {
				o.o->output(to, o.pos, false);
			} else {
				to << L"<invalid>";
			}
			return to;
		}

		Str *toS(EnginePtr e, OptionIter o) {
			return CREATE(Str, e.v, ::toS(o));
		}

	}
}
