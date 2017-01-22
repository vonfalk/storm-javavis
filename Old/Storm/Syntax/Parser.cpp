#include "stdafx.h"
#include "Parser.h"
#include "ParserTemplate.h"
#include "Exception.h"
#include "Shared/Format.h"
#include "Lib/ArrayTemplate.h"

#include <deque>
#include <algorithm>

namespace storm {
	namespace syntax {

		ParserBase::ParserBase() :
			rules(CREATE(MAP_PP(Rule, ArrayP<Option>), this)),
			emptyCache(CREATE(MAP_PV(Rule, Bool), this)),
			lastFinish(0) {

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
			to << L"Parser for " << r->name << L", currently using " << stateCount() << L" states = "
			   << toBytes(byteCount()) << L".";
		}

		Rule *ParserBase::rootRule() const {
			// This is verified in the constructor!
			ParserType *type = (ParserType *)myType;
			return type->root();
		}

		void ParserBase::addSyntax(Par<Package> pkg) {
			// We need to make sure the pkg is loaded before we can iterate through it.
			pkg->forceLoad();
			for (NameSet::Iter i = pkg->begin(), end = pkg->end(); i != end; ++i) {
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

		Nat ParserBase::stateCount() const {
			return stateAlloc.count();
		}

		Nat ParserBase::byteCount() const {
			return stateAlloc.count() * sizeof(State);
		}

		Bool ParserBase::hasError() const {
			if (lastFinish < steps.size() - 1)
				return true;

			// Tried to parse empty string?
			return finish() == null;
		}

		Bool ParserBase::hasTree() const {
			return finish() != null;
		}

		SyntaxError ParserBase::error() const {
			nat pos = lastStep();
			std::wostringstream msg;

			if (pos == steps.size() - 1)
				msg << L"Unexpected end of stream.\n";
			else
				msg << L"Unexpected '" << String::escape(src->v[pos]) << L"'.\n";

			msg << L"In progress:";

			typedef map<String, set<String>> Data;
			Data rules = inProgress(steps[pos]);
			for (Data::iterator i = rules.begin(); i != rules.end(); ++i) {
				msg << endl;
				join(msg, i->second, L"\n");
				msg << i->first;
			}

			return SyntaxError(srcPos + pos, msg.str());
		}

		map<String, set<String>> ParserBase::inProgress(const StateSet &step) const {
			map<String, set<String>> r;

			for (nat i = 0; i < step.count(); i++) {
				State *now = step[i];
				Rule *rule = now->pos.rulePtr();
				String ruleStr = ::toS(now->pos);

				// Find all states that instantiated this one.
				std::wostringstream from;
				const StateSet &src = steps[now->from];
				for (nat j = 0; j < src.count(); j++) {
					if (RuleToken *token = src[j]->getRule()) {
						if (token->rule.borrow() == rule) {
							from << L"\n from: " << src[j]->pos;
						}
					}
				}

				// Bundle all rules originating from the same place together.
				r[from.str()].insert(ruleStr);
			}

			return r;
		}

		void ParserBase::throwError() const {
			throw error();
		}

		Str *ParserBase::errorMsg() const {
			return CREATE(Str, this, error().what());
		}

		nat ParserBase::lastStep() const {
			for (nat i = steps.size() - 1; i > 0; i--) {
				if (steps[i].count() > 0)
					return i;
			}

			// First step is never empty.
			return 0;
		}


		/**
		 * Parsing logic.
		 */

		Str::Iter ParserBase::parse(Par<Str> str, SrcPos from, Str::Iter start) {
			// Remember what we parsed.
			src = str;
			srcPos = from;
			lastFinish = 0;

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
			steps[pos].push(State(rootOption->firstA(), pos, 0), stateAlloc);

			nat len = pos;
			for (nat i = pos; i < steps.size(); i++) {
				if (process(i)) {
					len = i;
				}
			}

			lastFinish = len;

			// Note: as we're not fully compliant with unicode, we may end up in the middle of a
			// surrogate pair here! This can be fixed by making the regex engine aware of surrogate
			// pairs.
			start.charIndex(len);
			return start;
		}

		bool ParserBase::process(nat step) {
			bool seenFinish = false;
			// if (debugParser)
			// 	PVAR(step);

			StateSet &src = steps[step];
			for (nat i = 0; i < src.count(); i++) {
				State *at = src[i];
				// if (debugParser)
				// 	PVAR(at);

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
				steps[step].push(State(option->firstA(), step, step), stateAlloc);
				steps[step].push(State(option->firstB(), step, step), stateAlloc);
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

					src.push(State(state->pos.nextA(), step, state->from, state, now), stateAlloc);
					src.push(State(state->pos.nextB(), step, state->from, state, now), stateAlloc);
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

				steps[step].push(State(st->pos.nextA(), step, st->from, st, state), stateAlloc);
				steps[step].push(State(st->pos.nextB(), step, st->from, st, state), stateAlloc);
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

			steps[matched].push(State(state->pos.nextA(), matched, state->from, state), stateAlloc);
			steps[matched].push(State(state->pos.nextB(), matched, state->from, state), stateAlloc);
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


		/**
		 * Generate the syntax tree.
		 */

		Node *ParserBase::tree() const {
			// Find the finish state...
			State *from = finish();
			if (!from)
				throw error();

			// The 'from' finishes the dummy 'rootOption', which the user is not interested in. The
			// user is interested in the option that finished 'rootOption', which is located in
			// 'from->completed'.
			assert(from->completed, L"The root state was not completed by anything!");
			return tree(from->completed);
		}

		Node *ParserBase::tree(State *from) const {
			Auto<Node> result = allocTreeNode(from);
			Option *option = from->pos.optionPtr();

			// Remember capture start and capture end. Set 'repStart' to first token since we will
			// not find it in the loop if the repeat starts at the first token in this option.
			nat repStart = from->from;
			nat repEnd = 0;

			// Traverse the states backwards. The last token in the chain (the one created first) is
			// skipped, as that does not contain any information.
			for (State *at = from; at->prev; at = at->prev) {
				State *prev = at->prev;
				Token *token = prev->pos.tokenPtr();

				if (at->pos.repStart())
					repStart = at->step;
				if (at->pos.repEnd())
					repEnd = at->step;

				// Don't bother if we do not need to store the result anywhere.
				if (!token->target)
					continue;

				int offset = token->target->offset().current();

				Object *match = null;
				if (as<RegexToken>(token)) {
					nat from = prev->step;
					nat to = at->step;

					match = CREATE(SStr, this, src->v.substr(from, to - from), srcPos + from);
				} else if (as<RuleToken>(token)) {
					assert(at->completed, L"Rule token not completed!");
					match = tree(at->completed);
				}

				Object *&dest = OFFSET_IN(result.borrow(), offset, Object *);

				if (isArray(token->target->varType)) {
					// Arrays are initialized earlier.
					ArrayP<Object> *array = (ArrayP<Object> *)dest;
					array->push(match);
				} else {
					dest = match;
				}
			}

			// Remember the capture.
			if (option->repCapture && option->repCapture->target && repStart <= repEnd) {
				Object *r = CREATE(SStr, this, src->v.substr(repStart, repEnd - repStart), srcPos + repStart);
				int offset = option->repCapture->target->offset().current();
				OFFSET_IN(result.borrow(), offset, Object *) = r;
			}

			// Reverse all arrays in this node, as we're adding them backwards.
			reverseTreeNode(from, result.borrow());

			return result.ret();
		}

		Node *ParserBase::allocTreeNode(State *from) const {
			OptionType *type = from->pos.optionPtr()->typePtr();

			// A bit ugly, but this is enough to be able to execute the destructor later on, and
			// when populated it is as if we have executed the regular constructor.
			void *mem = Object::operator new(type->size().current(), type);
			Node *r = new (mem) Node(srcPos + from->from);

			// Make 'r' into the correct subclass.
			setVTable(r);

			// Create any arrays needed.
			for (nat i = 0; i < type->arrayMembers.size(); i++) {
				TypeVar *v = type->arrayMembers[i];
				int offset = v->offset().current();
				Object *&dest = OFFSET_IN(r, offset, Object *);
				// This will actually create the correct subtype, as long as we're creating
				// something inherited from Object (which we are).
				dest = new (v->varType.type) ArrayP<Object>();
			}

			return r;
		}

		void ParserBase::reverseTreeNode(State *from, Node *node) const {
			OptionType *type = from->pos.optionPtr()->typePtr();

			for (nat i = 0; i < type->arrayMembers.size(); i++) {
				TypeVar *v = type->arrayMembers[i];
				int offset = v->offset().current();

				ArrayP<Object> *array = OFFSET_IN(node, offset, ArrayP<Object> *);
				array->reverse();
			}
		}

		State *ParserBase::finish() const {
			const StateSet &states = steps[lastFinish];
			for (nat i = 0; i < states.count(); i++) {
				State *s = states[i];
				if (s->finishes(rootOption))
					return s;
			}

			return null;
		}


		/**
		 * C++ interface.
		 */

		Parser::Parser() {}

		Parser *Parser::create(Par<Rule> root) {
			// Pick an appropriate type for it (!= the C++ type).
			Type *t = parserType(root->engine, Value(root));
			return new (t) Parser();
		}

		Parser *Parser::create(Par<Package> pkg, const String &name) {
			Auto<SimplePart> part = CREATE(SimplePart, pkg, name);
			Auto<Named> found = pkg->find(part);
			if (Rule *r = as<Rule>(found.borrow())) {
				return create(r);
			} else {
				throw InternalError(L"Can not find the rule " + name + L" in " + pkg->identifier());
			}
		}

		const void *transformFunction(Type *type, const Value &result, const Value &param) {
			vector<Value> par;
			par.push_back(Value::thisPtr(type));
			if (param != Value())
				par.push_back(param);
			Auto<SimplePart> part = CREATE(SimplePart, type, L"transform", par);
			Auto<Named> found = type->find(part);
			if (Function *fn = as<Function>(found.borrow())) {
				if (!result.canStore(fn->result)) {
					throw InternalError(L"The function " + fn->identifier() + L" returns " +
										::toS(fn->result) + L", which is incompatible with " +
										::toS(result));
				}

				return fn->pointer();
			} else {
				throw InternalError(L"Can not find " + ::toS(part) + L" in " + type->identifier());
			}
		}

	}
}
