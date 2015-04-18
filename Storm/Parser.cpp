#include "stdafx.h"
#include "Parser.h"

#include "SyntaxTransform.h"
#include "Package.h"
#include <iomanip>
#include <deque>

namespace storm {

	bool parserDebug = false;

	/**
	 * Parser implementation.
	 */

	Parser::Parser(Par<SyntaxSet> set, Par<Str> src, const SrcPos &pos)
		: syntax(set), srcStr(src), src(src->v), srcPos(pos), rootOption(SrcPos(), Scope(), L"") {}

	Parser::Parser(Par<SyntaxSet> set, Par<Str> src, Par<Url> file)
		: syntax(set), srcStr(src), src(src->v), srcPos(file, 0), rootOption(SrcPos(), Scope(), L"") {}

	Parser::State Parser::firstState() {
		return State(OptionIter::firstA(rootOption), 0, StatePtr(), StatePtr(), 0);
	}

	Parser::State Parser::completedState(const OptionIter &ri, nat from, const StatePtr &prev, const StatePtr &by) {
		int prio = state(by).pos.option().priority;
		return State(ri, from, prev, by, prio);
	}

	Parser::State Parser::predictedState(const OptionIter &ri, nat from) {
		return State(ri, from, StatePtr(), StatePtr(), 0);
	}

	Parser::State Parser::scannedState(const OptionIter &ri, nat from, const StatePtr &prev) {
		return State(ri, from, prev, StatePtr(), 0);
	}

	Nat Parser::noMatch() const {
		return NO_MATCH;
	}

	Nat Parser::parse(Par<Str> root) {
		return parse(root->v);
	}

	Nat Parser::parse(Par<Str> root, Nat pos) {
		return parse(root->v, pos);
	}

	nat Parser::parse(const String &rootType, nat pos) {
		assert(pos <= src.size());

		rootOption.clear();
		rootOption.add(new TypeToken(rootType, L"root"));

		steps = vector<StateSet>(src.size() + 1);
		steps[pos].insert(firstState());

		nat len = NO_MATCH;

		for (nat i = pos; i < steps.size(); i++) {
			if (process(i))
				len = i;
		}

		return len;
	}

	bool Parser::process(nat step) {
		bool seenFinish = false;
		StateSet &s = steps[step];

		for (nat i = 0; i < s.size(); i++) {
			StatePtr ptr(step, i);
			if (parserDebug)
				PLN(ptr << ": " << s[i]);

			predictor(s, s[i], ptr);
			completer(s, s[i], ptr);
			scanner(s, s[i], ptr);

			if (s[i].finish(&rootOption))
				seenFinish = true;
		}

		return seenFinish;
	}

	void Parser::predictor(StateSet &s, State state, StatePtr ptr) {
		SyntaxToken *token = state.pos.token();
		TypeToken *type = dynamic_cast<TypeToken*>(token);
		if (!type)
			return;

		const String &rule = type->type();
		SyntaxRule *sr = syntax->rule(rule);
		if (sr == null)
			throw SyntaxError(state.pos.option().pos, L"Can not find rule " + type->type());

		SyntaxRule &t = *sr;

		for (nat i = 0; i < t.size(); i++) {
			SyntaxOption *rule = t[i];
			// Todo: We need to find possible lookahead strings!
			s.insert(predictedState(OptionIter::firstA(*rule), ptr.step));
			s.insert(predictedState(OptionIter::firstB(*rule), ptr.step));
		}

		if (matchesEmpty(t)) {
			// The original parser fails with rules like:
			// Foo => void : "(" - DELIMITER - Bar - DELIMITER - ")";
			// Bar => void : DELIMITER;
			// since the completed state of Bar may already have been added
			// and processed when trying to match DELIMITER. Therefore, we
			// need to look for completed instances of Bar (since it may match "")
			// in the current state before continuing. If it is not found, it
			// may be added and processed later, but that is OK.

			for (nat i = 0; i < ptr.id; i++) {
				const State &now = s[i];
				if (!now.pos.end())
					continue;

				if (rule != now.pos.option().rule())
					continue;

				StatePtr completedBy(ptr.step, i);
				State ns = completedState(state.pos.nextA(), state.from, ptr, completedBy);
				s.insert(ns);
				// if (ns.pos.valid())
				// 	PLN("=>" << ns);

				ns.pos = state.pos.nextB();
				s.insert(ns);
				// if (ns.pos.valid())
				// 	PLN("=>" << ns);
			}
		}
	}

	void Parser::scanner(StateSet &s, State state, StatePtr ptr) {
		SyntaxToken *token = state.pos.token();
		RegexToken *t = as<RegexToken>(token);
		if (!t)
			return;

		nat matched = t->regex.match(src, ptr.step);
		if (matched == NO_MATCH) {
			return;
		}

		// Should not happen, but best to be safe!
		if (matched >= steps.size())
			return;

		State ns = scannedState(state.pos.nextA(), state.from, ptr);
		steps[matched].insert(ns);

		ns.pos = state.pos.nextB();
		steps[matched].insert(ns);
	}

	void Parser::completer(StateSet &s, State state, StatePtr ptr) {
		if (!state.pos.end())
			return;

		String completed = state.pos.option().rule();
		StateSet &from = steps[state.from];
		for (nat i = 0; i < from.size(); i++) {
			State st = from[i];
			StatePtr stPtr(state.from, i);
			if (!st.isRule(completed))
				continue;

			int prioa = this->state(ptr).pos.option().priority;
			State ns(st.pos.nextA(), st.from, stPtr, ptr, prioa);
			s.insert(ns);
			// if (ns.pos.valid())
			// 	PLN("->" << ns);

			ns.pos = st.pos.nextB();
			s.insert(ns);
			// if (ns.pos.valid())
			// 	PLN("->" << ns);
		}
	}

	bool Parser::matchesEmpty(SyntaxRule &rule) {
		EmptyCache::iterator i = emptyCache.find(&rule);
		if (i != emptyCache.end())
			return i->second;

		// Tell it matches nothing in case the rule is recursive!
		// This will yeild correct results and prevent endless recursion.
		i = emptyCache.insert(make_pair(&rule, true)).first;

		bool result = false;
		for (nat i = 0; i < rule.size(); i++) {
			if (matchesEmpty(rule[i])) {
				result = true;
				break;
			}
		}

		i->second = result;
		return result;
	}

	static void insert(std::deque<OptionIter> &q, const OptionIter &next) {
		if (!next.valid())
			return;

		for (std::deque<OptionIter>::iterator i = q.begin(), end = q.end(); i != end; ++i)
			if (*i == next)
				return;
		q.push_back(next);
	}

	bool Parser::matchesEmpty(SyntaxOption *option) {
		std::deque<OptionIter> q;
		q.push_back(OptionIter::firstA(*option));
		q.push_back(OptionIter::firstB(*option));

		while (!q.empty()) {
			OptionIter now = q.front(); q.pop_front();
			if (!now.valid())
				continue;
			if (now.end())
				return true;

			if (matchesEmpty(now.token())) {
				insert(q, now.nextA());
				insert(q, now.nextB());
			}
		}

		return false;
	}

	bool Parser::matchesEmpty(SyntaxToken *token) {
		if (RegexToken *t = as< RegexToken>(token)) {
			return t->regex.match(L"") != NO_MATCH;

		} else if (TypeToken *t = as<TypeToken>(token)) {
			if (SyntaxRule *r = syntax->rule(t->type()))
				return matchesEmpty(*r);

			// Will not go well later on anyway if this result is ever needed.
			return true;
		} else {
			assert(false, "Unknown syntax token type.");
			return false;
		}
	}

	void Parser::throwError() const {
		throw error();
	}

	Str *Parser::errorMsg() const {
		return CREATE(Str, this, error().what());
	}

	Bool Parser::hasError() const {
		if (lastStep() < steps.size() - 1) {
			return true;
		}

		const StateSet &s = steps.back();
		for (nat i = 0; i < s.size(); i++)
			if (s[i].finish(&rootOption))
				return false;

		return true;
	}

	SyntaxError Parser::error() const {
		nat pos = lastStep();
		std::wostringstream oss;

		if (pos == steps.size() - 1)
			oss << L"Unexpected end of stream.";
		else
			oss << L"Unexpected " << String::escape(src[pos]);

		set<String> tokens = typeCompletions(steps[pos]);
		if (!tokens.empty()) {
			oss << L"\nExpected types: ";
			join(oss, tokens, L", ");
		}

		set<String> regexes = regexCompletions(steps[pos]);
		if (!regexes.empty()) {
			oss << L"\nExpected tokens: \"";
			join(oss, regexes, L"\", \"");
			oss << L"\"";
		}

		set<String> rules = rulesInProgress(steps[pos]);
		if (!rules.empty()) {
			oss << L"\nRules in progress: ";
			join(oss, rules, L", ");
		}

		return SyntaxError(srcPos + pos, oss.str());
	}

	nat Parser::lastStep() const {
		for (nat i = steps.size() - 1; i > 0; i--) {
			if (steps[i].size() != 0)
				return i;
		}
		return 0;
	}

	Parser::StatePtr Parser::finish() const {
		for (nat i = steps.size(); i > 0; i--) {
			for (nat j = 0; j < steps[i-1].size(); j++) {
				if (steps[i-1][j].finish(&rootOption))
					return StatePtr(i-1, j);
			}
		}
		return StatePtr();
	}

	set<String> Parser::typeCompletions(const StateSet &states) const {
		set<String> r;

		for (nat i = 0; i < states.size(); i++) {
			if (states[i].isRule())
				r.insert(states[i].tokenRule());
		}

		return r;
	}

	set<String> Parser::regexCompletions(const StateSet &states) const {
		set<String> r;

		for (nat i = 0; i < states.size(); i++) {
			if (states[i].isRegex())
				r.insert(::toS(states[i].tokenRegex()));
		}

		return r;
	}

	set<String> Parser::rulesInProgress(const StateSet &states) const {
		set<String> r;

		for (nat i = 0; i < states.size(); i++) {
			const String &n = states[i].pos.option().rule();
			if (n != L"")
				r.insert(n);
		}

		return r;
	}

	Parser::State &Parser::state(const StatePtr &ptr) {
		assert(ptr.valid());
		return steps[ptr.step][ptr.id];
	}


	void Parser::dbgPrintTree(StatePtr ptr, nat indent) {
		State &s = state(ptr);

		String in(indent, ' ');
		PLN(in << ptr << ": " << s.pos);

		if (s.prev.valid())
			dbgPrintTree(s.prev, indent + 1);
		if (s.completed.valid())
			dbgPrintTree(s.completed, indent + 1);
	}

	SyntaxNode *Parser::tree() {
		StatePtr f = finish();
		if (!f.valid()) {
			WARNING("Getting tree from an unfinished tree!");
			return null;
		}

		SyntaxNode *result = null;

		SyntaxNode *root = tree(f);
		// SyntaxNode::Var &resultVar = root->find(L"root", SyntaxVariable::tNode);
		const SyntaxNode::Var *resultVar = root->find(L"root");
		if (resultVar->value) {
			result = resultVar->value->node();
			resultVar->value->orphan();
		}
		delete root;

		return result;
	}

	SyntaxNode *Parser::tree(StatePtr pos) {
		State *cState = &state(pos);
		SyntaxNode *result = null;
		SyntaxNode *tmp = null;
		nat captureBegin = 0, captureEnd = 0;
		SyntaxOption &option = cState->pos.option();

		try {
			result = new SyntaxNode(&option);

			for (; cState->prev.valid(); pos = cState->prev, cState = &state(pos)) {
				if (cState->pos.captureEnd())
					captureEnd = pos.step;
				if (cState->pos.captureBegin())
					captureBegin = pos.step;

				State *pState = &state(cState->prev);

				if (pState->bindToken() || pState->invokeToken()) {
					const String &to = pState->bindToken() ?
						pState->bindTokenTo() : pState->invokeOnToken();

					vector<String> params;
					if (TypeToken *t = as<TypeToken>(pState->pos.token()))
						params = t->params;

					SrcPos p = srcPos + cState->prev.step;

					if (cState->completed.valid()) {
						tmp = tree(cState->completed);
						if (pState->bindToken())
							result->add(to, tmp, params, p);
						else
							result->invoke(to, tmp, params, p);
						tmp = null;
					} else {
						nat fromPos = cState->prev.step;
						nat toPos = pos.step;
						String matched = src.substr(fromPos, toPos - fromPos);
						if (pState->bindToken())
							result->add(to, matched, params, p);
						else
							result->invoke(to, matched, params, p);
					}
				}
			}

			if (cState->pos.captureBegin())
				captureBegin = pos.step;

			result->reverseArrays();
		} catch (...) {
			delete result;
			delete tmp;
			throw;
		}

		if (option.hasCapture() && captureBegin <= captureEnd) {
			SrcPos p = srcPos + captureBegin;
			String captured = src.substr(captureBegin, captureEnd - captureBegin);
			result->add(option.captureTo(), captured, vector<String>(), p);
		}

		return result;
	}

	/**
	 * Shorthand.
	 */

	Object *Parser::transform(const vector<Object *> &params) {
		SyntaxNode *root = tree();
		if (!root)
			return null;

		try {
			Auto<Object> result = storm::transform(engine(), *syntax, *root, params);
			delete root;
			return result.ret();
		} catch (...) {
			delete root;
			throw;
		}
	}

	Object *Parser::transform(Par<ArrayP<Object>> p) {
		vector<Object *> params(p->count());
		for (nat i = 0; i < p->count(); i++)
			params[i] = p->at(i).borrow();

		return transform(params);
	}

	TObject *Parser::transformT(Par<ArrayP<TObject>> p) {
		vector<Object *> params(p->count());
		for (nat i = 0; i < p->count(); i++)
			params[i] = p->at(i).borrow();

		Auto<Object> o = transform(params);
		if (!dynamic_cast<TObject *>(o.borrow()))
			throw InternalError(L"Expected a TObject while transforming syntax.");
		return (TObject *)o.ret();
	}

	/**
	 * State.
	 */
	void Parser::State::output(wostream &to) const {
		to << "{State: " << pos << ", from: " << from;
		to << ", prev: " << prev << ", completed: " << completed << "[" << completedPriority << "]}";
	}

	bool Parser::State::isRule() const {
		SyntaxToken *token = pos.token();
		TypeToken *t = dynamic_cast<TypeToken*>(token);
		return t ? true : false;
	}

	bool Parser::State::isRule(const String &name) const {
		return isRule() && tokenRule() == name;
	}

	const String &Parser::State::tokenRule() const {
		assert(isRule());
		SyntaxToken *token = pos.token();
		TypeToken *t = static_cast<TypeToken*>(token);
		return t->type();
	}

	bool Parser::State::isRegex() const {
		SyntaxToken *token = pos.token();
		RegexToken *t = dynamic_cast<RegexToken*>(token);
		return t ? true : false;
	}

	const Regex &Parser::State::tokenRegex() const {
		assert(isRegex());
		SyntaxToken *token = pos.token();
		RegexToken *t = static_cast<RegexToken*>(token);
		return t->regex;
	}

	bool Parser::State::bindToken() const {
		return pos.token()->bind();
	}

	const String &Parser::State::bindTokenTo() const {
		return pos.token()->bindTo;
	}

	bool Parser::State::invokeToken() const {
		return pos.token()->invoke();
	}

	const String &Parser::State::invokeOnToken() const {
		return pos.token()->bindTo;
	}

	bool Parser::State::finish(const SyntaxOption *rootOption) const {
		return &pos.option() == rootOption
			&& pos.end();
		// We could check pos.from here as well, but we can never instantiate that rule more than once!
	}

	/**
	 * State set.
	 */

	void Parser::StateSet::insert(const State &state) {
		if (!state.pos.valid())
			return;

		for (nat i = 0; i < size(); i++) {
			State &c = (*this)[i];
			if (c == state) {
				int cPrio = c.completedPriority;
				int sPrio = state.completedPriority;
				if (cPrio > sPrio) {
					c = state;
				}
				return;
			}
		}

		push_back(state);
		return;
	}

	/**
	 * StatePtr.
	 */

	void Parser::StatePtr::output(wostream &to) const {
		if (valid())
			to << L"(" << step << L", " << id << L")";
		else
			to << L"(-)";
	}
}
