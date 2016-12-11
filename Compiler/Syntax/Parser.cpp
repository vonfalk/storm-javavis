#include "stdafx.h"
#include "Parser.h"
#include "Package.h"
#include "Core/Runtime.h"
#include "Lib/ParserTemplate.h"

namespace storm {
	namespace syntax {

		ParserBase::ParserBase() {
			rules = new (this) Map<Rule *, RuleInfo>();
			lastFinish = -1;

			assert(as<ParserType>(runtime::typeOf(this)),
				L"ParserBase not properly constructed. Use Parser::create() in C++!");

			// Find the package where 'root' is located and add that!
			if (Package *pkg = ScopeLookup::firstPkg(root())) {
				addSyntax(pkg);
			} else {
				WARNING(L"The rule " << root() << L" is not located in any package. No default package added!");
			}
		}

		void ParserBase::toS(StrBuf *to) const {
			// TODO: use 'toBytes' when present!
			*to << L"Parser for " << root()->identifier() << L", currently using " << stateCount()
				<< L" states = " << byteCount() << L" bytes.";
		}

		Nat ParserBase::stateCount() const {
			Nat r = 0;
			for (nat i = 0; i < steps->count(); i++)
				r += steps->at(i).count();
			return r;
		}

		Nat ParserBase::byteCount() const {
			return sizeof(ParserBase) + steps->count() * sizeof(void *) + stateCount() * sizeof(State);
		}

		Rule *ParserBase::root() const {
			// This is verified in the constructor.
			ParserType *type = (ParserType *)runtime::typeOf(this);
			return type->root;
		}

		void ParserBase::addSyntax(Package *pkg) {
			pkg->forceLoad();
			for (NameSet::Iter i = pkg->begin(), e = pkg->end(); i != e; ++i) {
				Named *n = i.v();
				if (Rule *rule = as<Rule>(n)) {
					// We only keep track of these to get better error messages!
					// Make sure to create an entry for the rule!
					rules->at(rule);
				} else if (ProductionType *t = as<ProductionType>(n)) {
					Rule *owner = as<Rule>(t->super());
					if (!owner)
						throw InternalError(::toS(t->identifier()) + L" is not defined correctly.");
					rules->at(owner).push(t->production);
				}
			}
		}

		void ParserBase::addSyntax(Array<Package *> *pkgs) {
			for (nat i = 0; i < pkgs->count(); i++)
				addSyntax(pkgs->at(i));
		}

		Bool ParserBase::parse(Str *str, Url *file) {
			return parse(str, file, str->begin());
		}

		Bool ParserBase::parse(Str *str, Url *file, Str::Iter start) {
			// Remember what we parsed.
			src = str;
			srcPos = SrcPos(file, start.offset());
			Nat len = str->peekLength() + 1;
			lastFinish = len + 1;

			// Set up storage.
			steps = new (this) Array<StateSet>();
			steps->reserve(len);
			for (nat i = 0; i < len; i++)
				steps->push(StateSet(engine()));

			// Create the initial production.
			rootProd = new (this) Production();
			rootProd->tokens->push(new (this) RuleToken(root()));

			// Set up the initial state.
			steps->at(0).push(rootProd->firstA(), 0);

			// Process all steps.
			for (nat i = 0; i < len; i++) {
				//PVAR(i);
				if (process(i))
					lastFinish = i;
			}

			return lastFinish < len;
		}

		bool ParserBase::process(nat step) {
			bool seenFinish = false;

			StateSet &src = steps->at(step);
			for (nat i = 0; i < src.count(); i++) {
				State *at = src[i];
				//PVAR(at);

				predictor(step, at);
				completer(step, at);
				scanner(step, at);

				if (at->finishes(rootProd))
					seenFinish = true;
			}

			return seenFinish;
		}

		void ParserBase::predictor(nat step, State *state) {
			RuleToken *rule = state->getRule();
			if (!rule)
				return;

			// Note: this creates new (empty) entries for any rules referred to but not yet included.
			RuleInfo &info = rules->at(rule->rule);

			for (Nat i = 0; i < info.productions->count(); i++) {
				Production *p = info.productions->at(i);
				steps->at(step).push(p->firstA(), step);
				steps->at(step).push(p->firstB(), step);
			}

			if (matchesEmpty(info)) {
				// The original parser fails with rules like:
				// DELIMITER : " *";
				// Foo : "(" - DELIMITER - Bar - DELIMITER - ")";
				// Bar : DELIMITER;
				// since the completed state of Bar may already have been added and processed when
				// trying to match DELIMITER. Therefore, we need to look for completed instances of
				// Bar (since it may match "") in the current state before continuing. If it is not
				// found, it will be added and processed later, which is OK. This is basically what
				// the completer does (only less general):

				StateSet &src = steps->at(step);
				// We do not need to examine further than the current state. Anything else will be
				// examined in the main loop later.
				for (nat i = 0; src[i] != state && i < src.count(); i++) {
					State *now = src[i];
					if (!now->pos.end())
						continue;

					Rule *cRule = now->production()->rule();
					if (cRule != rule->rule)
						continue;

					src.push(state->pos.nextA(), state->from, state, now);
					src.push(state->pos.nextB(), state->from, state, now);
				}
			}
		}

		void ParserBase::completer(nat step, State *state) {
			if (!state->pos.end())
				return;

			Rule *completed = state->pos.rule();
			StateSet &from = steps->at(state->from);
			for (nat i = 0; i < from.count(); i++) {
				State *s = from[i];
				RuleToken *rule = s->getRule();
				if (!rule)
					continue;
				if (rule->rule != completed)
					continue;

				steps->at(step).push(s->pos.nextA(), s->from, state);
				steps->at(step).push(s->pos.nextB(), s->from, state);
			}
		}

		void ParserBase::scanner(nat step, State *state) {
			RegexToken *regex = state->getRegex();
			if (!regex)
				return;

			nat offset = srcPos.pos;
			nat matched = regex->regex.matchRaw(src, step + offset);
			if (matched == Regex::NO_MATCH)
				return;

			// Should not happen, but better safe than sorry!
			if (matched < offset)
				return;
			matched -= offset;
			if (matched > steps->count())
				return;

			steps->at(matched).push(state->pos.nextA(), state->from, state);
			steps->at(matched).push(state->pos.nextB(), state->from, state);
		}

		bool ParserBase::matchesEmpty(Rule *rule) {
			return matchesEmpty(rules->at(rule));
		}

		bool ParserBase::matchesEmpty(RuleInfo &info) {
			if (info.matchesNull < 2)
				return info.matchesNull != 0;

			// Say the rule matches null in case it is recursive!
			// This will provide correct results and prevent endless recursion.
			info.matchesNull = 1;

			bool match = false;
			for (Nat i = 0; i < info.productions->count(); i++) {
				if (matchesEmpty(info.productions->at(i))) {
					match = true;
					break;
				}
			}

			info.matchesNull = match ? 1 : 0;
			return match;
		}

		static const ProductionIter &pick(const ProductionIter &a, const ProductionIter &b) {
			if (!b.valid())
				return a;
			if (!a.valid())
				return b;
			if (a.position() < b.position())
				return b;
			else
				return a;
		}

		bool ParserBase::matchesEmpty(Production *p) {
			// Note: since we try to match against the empty string, we can greedily pick the one of
			// nextA and nextB which is furthest along the production at each step.
			ProductionIter pos = pick(p->firstA(), p->firstB());
			while (pos.valid() && !pos.end()) {
				if (!matchesEmpty(pos.token()))
					return false;

				pos = pick(pos.nextA(), pos.nextB());
			}

			return true;
		}

		bool ParserBase::matchesEmpty(Token *t) {
			if (RegexToken *r = as<RegexToken>(t)) {
				return r->regex.matchRaw(new (this) Str(L"")) != Regex::NO_MATCH;
			} else if (RuleToken *u = as<RuleToken>(t)) {
				return matchesEmpty(u->rule);
			} else {
				assert(false, L"Unknown syntax token type.");
				return false;
			}
		}


		/**
		 * Rule info struct.
		 */

		ParserBase::RuleInfo::RuleInfo() : productions(null), matchesNull(2) {}

		void ParserBase::RuleInfo::push(Production *p) {
			if (!productions)
				productions = new (p) Array<Production *>();
			productions->push(p);
		}


		/**
		 * C++ interface.
		 */

		Parser::Parser() {}

		Parser *Parser::create(Rule *root) {
			// Pick an appropriate type for it (!= the C++ type).
			Type *t = parserType(root);
			void *mem = runtime::allocObject(sizeof(Parser), t);
			Parser *r = new (Place(mem)) Parser();
			t->vtable->insert(r);
			return r;
		}

		Parser *Parser::create(Package *pkg, const wchar *name) {
			if (Rule *r = as<Rule>(pkg->find(name))) {
				return create(r);
			} else {
				throw InternalError(L"Can not find the rule " + ::toS(name) + L" in " + ::toS(pkg->identifier()));
			}
		}

	}
}
