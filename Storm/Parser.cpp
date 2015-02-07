#include "stdafx.h"
#include "Parser.h"

#include "SyntaxTransform.h"
#include "Package.h"
#include <iomanip>

namespace storm {

	/**
	 * Parser implementation.
	 */

	Parser::Parser(SyntaxSet &set, const String &src, const SrcPos &pos)
		: syntax(set), src(src), srcPos(pos), rootOption(SrcPos(), Scope(), L"") {}

	Parser::Parser(SyntaxSet &set, const String &src, const Path &file)
		: syntax(set), src(src), srcPos(file, 0), rootOption(SrcPos(), Scope(), L"") {}

	nat Parser::parse(const String &rootType, nat pos) {
		assert(pos <= src.size());

		rootOption.clear();
		rootOption.add(new TypeToken(rootType, L"root"));

		steps = vector<StateSet>(src.size() + 1);
		steps[pos].insert(State(OptionIter::firstA(rootOption), 0));

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
			// PLN(ptr << ": " << s[i]);

			predictor(s, s[i], ptr);
			completer(s, s[i], ptr);
			scanner(s, s[i], ptr);

			if (s[i].finish(&rootOption))
				seenFinish = true;

			if (i == s.size() - 1) {
				// Run completers again, some may produce more states now.
				runCompleters(step);
			}
		}

		return seenFinish;
	}

	void Parser::runCompleters(nat step) {
		StateSet &s = steps[step];
		nat to = s.size();
		for (nat i = 0; i < to; i++) {
			StatePtr ptr(step, i);
			completer(s, s[i], ptr);
		}
	}

	void Parser::predictor(StateSet &s, State state, StatePtr ptr) {
		SyntaxToken *token = state.pos.token();
		TypeToken *type = dynamic_cast<TypeToken*>(token);
		if (!type)
			return;

		SyntaxRule *sr = syntax.rule(type->type());
		if (sr == null)
			throw SyntaxError(state.pos.option().pos, L"Can not find rule " + type->type());

		SyntaxRule &t = *sr;

		for (nat i = 0; i < t.size(); i++) {
			SyntaxOption *rule = t[i];
			// Todo: We need to find possible lookahead strings!
			s.insert(State(OptionIter::firstA(*rule), ptr.step));
			s.insert(State(OptionIter::firstB(*rule), ptr.step));
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

		State ns(state.pos.nextA(), state.from, ptr);
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

			State ns(st.pos.nextA(), st.from, stPtr, ptr);
			s.insert(ns);
			// if (ns.pos.valid())
			// 	PLN("->" << ns);

			ns.pos = st.pos.nextB();
			s.insert(ns);
			// if (ns.pos.valid())
			// 	PLN("->" << ns);
		}
	}

	bool Parser::hasError() const {
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
			oss << L"Unexpected " << src[pos];

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
				r.insert(toS(states[i].tokenRegex()));
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
		if (!f.valid())
			return null;

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

		if (option.hasCapture() && captureBegin < captureEnd) {
			SrcPos p = srcPos + captureBegin;
			String captured = src.substr(captureBegin, captureEnd - captureBegin);
			result->add(option.captureTo(), captured, vector<String>(), p);
		}

		return result;
	}

	/**
	 * Shorthand.
	 */

	Object *Parser::transform(Engine &engine, const vector<Object *> &params) {
		SyntaxNode *root = tree();
		if (!root)
			return null;

		try {
			Auto<Object> result = storm::transform(engine, syntax, *root, params);
			delete root;
			return result.ret();
		} catch (...) {
			delete root;
			throw;
		}
	}

	/**
	 * State.
	 */
	void Parser::State::output(wostream &to) const {
		to << "{State: " << pos << ", from: " << from;
		to << ", prev: " << prev << ", completed: " << completed << "}";
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

	bool Parser::StateSet::insert(const State &state) {
		if (!state.pos.valid())
			return true;

		for (nat i = 0; i < size(); i++)
			if ((*this)[i] == state)
				return false;

		push_back(state);
		return true;
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
