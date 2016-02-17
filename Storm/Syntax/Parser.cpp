#include "stdafx.h"
#include "Parser.h"
#include "ParserTemplate.h"
#include "Exception.h"

#include <deque>
#include <algorithm>

namespace storm {
	namespace syntax {

		ParserBase::ParserBase() :
			rules(CREATE(MAP_PP(Rule, ArrayP<Option>), this)),
			emptyCache(CREATE(MAP_PV(Rule, Bool), this)) {

			ParserType *type = as<ParserType>(myType);
			assert(type, L"An instance of ParserBase was not correctly created. Use Parser instead!");

			// Find the package in which 'type' is located.
			Auto<Rule> root = rootRule();
			Package *pkg = ScopeLookup::firstPkg(root.borrow());
			if (pkg) {
				addSyntax(pkg);
			} else {
				WARNING(L"The rule " << root << L" is not located in any package, not adding one by default.");
			}
		}

		void ParserBase::output(wostream &to) const {
			Auto<Rule> r = rootRule();
			to << L"Parser: " << r->name;
		}

		Rule *ParserBase::rootRule() const {
			// This is verified in the constructor!
			ParserType *type = (ParserType *)myType;
			return type->root();
		}

		void ParserBase::addSyntax(Par<Package> pkg) {
			// We need to make sure the pkg is loaded before we can iterate through it.
			pkg->forceLoad();
			for (NameSet::iterator i = pkg->begin(), end = pkg->end(); i != end; ++i) {
				Named *n = i->borrow();
				if (Rule *rule = as<Rule>(n)) {
					// We only keep track of these to get better error messages!
					// Make sure to create an entry for the rule!
					options(rule);
				} else if (OptionType *option = as<OptionType>(n)) {
					Rule *owner = as<Rule>(option->super());
					if (!owner)
						throw InternalError(L"The option " + option->identifier() + L" is not correctly defined!");

					ArrayP<Option> *to = options(owner);
					to->push(option->option);
				}
			}
		}

		ArrayP<Option> *ParserBase::options(Par<Rule> rule) {
			if (rules->has(rule)) {
				return rules->get(rule).borrow();
			} else {
				Auto<ArrayP<Option>> r = CREATE(ArrayP<Option>, this);
				rules->put(rule, r);
				return r.borrow();
			}
		}

		void ParserBase::addSyntax(Par<ArrayP<Package>> pkg) {
			for (Nat i = 0; i < pkg->count(); i++) {
				addSyntax(pkg->at(i));
			}
		}

		Str::Iter ParserBase::parse(Par<Str> str, SrcPos from) {
			return parse(str, from, str->begin());
		}

		Str::Iter ParserBase::parse(Par<Str> str, Par<Url> file) {
			return parse(str, file, str->begin());
		}

		Str::Iter ParserBase::parse(Par<Str> str, Par<Url> file, Str::Iter start) {
			return parse(str, SrcPos(file, 0), start);
		}

		Bool ParserBase::hasError() {
			return false;
		}

		SyntaxError ParserBase::error() const {
			return SyntaxError(SrcPos(), L"TODO!");
		}

		void ParserBase::throwError() const {
			throw error();
		}

		Str *ParserBase::errorMsg() const {
			return CREATE(Str, this, error().what());
		}


		/**
		 * Parsing logic.
		 */

		Str::Iter ParserBase::parse(Par<Str> str, SrcPos from, Str::Iter start) {
			// Remember what we parsed.
			src = str;
			srcPos = from;

			// Set up storage.
			stateAlloc.clear();
			steps = vector<StateSet>(src->v.size() + 1);

			// Create the root rule to begin parsing.
			rootOption = CREATE(Option, this);
			{
				Auto<Rule> rule = rootRule();
				Auto<Token> rootToken = CREATE(RuleToken, this, rule);
				rootOption->tokens->push(rootToken);
			}

			// Anything to parse?
			nat pos = start.charIndex();
			if (pos >= steps.size())
				return str->end();

			// Set up the initial state.
			steps[pos].push(State(rootOption->firstA(), 0), stateAlloc);

			nat len = pos;
			for (nat i = pos; i < steps.size(); i++) {
				if (process(i))
					len = i;
			}

			PLN(L"Used " << stateAlloc.count() << L" entries.");
			PVAR(len);
			// Note: as we're not fully compliant with unicode, we may end up in the middle of a
			// surrogate pair here! This can be fixed by making the regex engine aware of surrogate
			// pairs.
			start.charIndex(len);
			return start;
		}

		bool ParserBase::process(nat step) {
			bool seenFinish = false;
			// PVAR(step);

			StateSet &src = steps[step];
			for (nat i = 0; i < src.count(); i++) {
				State *at = src[i];
				// PVAR(at);

				predictor(step, at);
				completer(step, at);
				scanner(step, at);

				if (at->finishes(rootOption))
					seenFinish = true;
			}

			return seenFinish;
		}

		void ParserBase::predictor(nat step, State *state) {
			RuleToken *rule = state->getRule();
			if (!rule)
				return;

			if (!rules->has(rule->rule)) {
				// If we get here, that means that the rule referred to is declared in another
				// package along with any options it has. Either way, this is equivalent to having
				// no options visible, which is not an error.
				return;
			}

			ArrayP<Option> *opts = rules->get(rule->rule).borrow();
			for (Nat i = 0; i < opts->count(); i++) {
				Option *option = opts->at(i).borrow();
				steps[step].push(State(option->firstA(), step), stateAlloc);
				steps[step].push(State(option->firstB(), step), stateAlloc);
			}

			if (matchesEmpty(rule->rule)) {
				// The original parser fails with rules like:
				// DELIMITER : " *";
				// Foo : "(" - DELIMITER - Bar - DELIMITER - ")";
				// Bar : DELIMITER;
				// since the completed state of Bar may already have been added
				// and processed when trying to match DELIMITER. Therefore, we
				// need to look for completed instances of Bar (since it may match "")
				// in the current state before continuing. If it is not found, it may
				// be added and processed later, which is OK.

				StateSet &src = steps[step];
				// We do not need to examine further than the current state. Anything else will be
				// examined in the main loop later.
				for (nat i = 0; src[i] != state && i < src.count(); i++) {
					State *now = src[i];
					if (!now->pos.end())
						continue;

					Rule *cRule = now->pos.rulePtr();
					if (cRule != rule->rule.borrow())
						continue;

					src.push(State(state->pos.nextA(), state->from, state, now), stateAlloc);
					src.push(State(state->pos.nextB(), state->from, state, now), stateAlloc);
				}
			}
		}

		void ParserBase::completer(nat step, State *state) {
			if (!state->pos.end())
				return;

			Rule *completed = state->pos.rulePtr();
			StateSet &from = steps[state->from];
			for (nat i = 0; i < from.count(); i++) {
				State *st = from[i];
				RuleToken *rule = st->getRule();
				if (!rule)
					continue;
				if (rule->rule.borrow() != completed)
					continue;

				steps[step].push(State(st->pos.nextA(), st->from, st, state), stateAlloc);
				steps[step].push(State(st->pos.nextB(), st->from, st, state), stateAlloc);
			}
		}

		void ParserBase::scanner(nat step, State *state) {
			RegexToken *regex = state->getRegex();
			if (!regex)
				return;

			nat matched = regex->regex.match(src->v, step);
			if (matched == storm::NO_MATCH)
				return;

			// Should not happen, but best to be safe!
			if (matched >= steps.size())
				return;

			steps[matched].push(State(state->pos.nextA(), state->from, state), stateAlloc);
			steps[matched].push(State(state->pos.nextB(), state->from, state), stateAlloc);
		}

		bool ParserBase::matchesEmpty(const Auto<Rule> &rule) {
			if (emptyCache->has(rule))
				return emptyCache->get(rule);

			if (!rules->has(rule))
				return false;
			ArrayP<Option> *opts = rules->get(rule).borrow();

			// Tell it matches nothing in case the rule is recursive!
			// This will yeild correct results and prevent endless recursion.
			emptyCache->put(rule, true);

			bool result = false;
			for (Nat i = 0; i < opts->count(); i++) {
				if (matchesEmpty(opts->at(i))) {
					result = true;
					break;
				}
			}

			emptyCache->put(rule, result);
			return result;
		}

		static void insert(std::deque<OptionIter> &q, const OptionIter &next) {
			if (!next.valid())
				return;

			if (std::find(q.begin(), q.end(), next) == q.end())
				q.push_back(next);
		}

		bool ParserBase::matchesEmpty(Par<Option> opt) {
			std::deque<OptionIter> q;
			insert(q, opt->firstA());
			insert(q, opt->firstB());

			while (!q.empty()) {
				OptionIter now = q.front(); q.pop_front();
				if (now.end())
					return true;

				if (matchesEmpty(now.tokenPtr())) {
					insert(q, now.nextA());
					insert(q, now.nextB());
				}
			}

			return false;
		}

		bool ParserBase::matchesEmpty(Par<Token> tok) {
			if (RegexToken *r = as<RegexToken>(tok.borrow())) {
				return r->regex.match(L"") != storm::NO_MATCH;
			} else if (RuleToken *u = as<RuleToken>(tok.borrow())) {
				return matchesEmpty(u->rule);
			} else {
				assert(false, L"Unknown syntax token type.");
				return false;
			}
		}

	}
}
