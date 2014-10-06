#include "stdafx.h"
#include "Parser.h"

#include "Package.h"
#include <iomanip>

namespace storm {

	void SyntaxSet::add(Package &pkg) {
		assert(syntax.size() == 0); // No support for merging yet!
		syntax = pkg.syntax();
	}

	/**
	 * Parser implementation.
	 */

	Parser::Parser(SyntaxSet &set, const String &src) : syntax(set), src(src), rootRule(SrcPos()) {}

	nat Parser::parse(const String &rootType, nat pos) {
		rootRule.clear();
		rootRule.add(new TypeToken(rootType, L"root"));

		steps = vector<StateSet>(src.size() + 1);
		steps[pos].insert(State(rootRule, 0));

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

			predictor(s, s[i], ptr);
			completer(s, s[i], ptr);
			scanner(s, s[i], ptr);

			if (s[i].finish(&rootRule))
				seenFinish = true;
		}

		return seenFinish;
	}

	void Parser::predictor(StateSet &s, State state, StatePtr ptr) {
		SyntaxToken *token = state.pos.token();
		TypeToken *type = dynamic_cast<TypeToken*>(token);
		if (!type)
			return;

		if (syntax.syntax.count(type->type()) == 0)
			return;
		SyntaxType &t = *syntax.syntax[type->type()];

		for (nat i = 0; i < t.size(); i++) {
			SyntaxRule *rule = t[i];
			// Todo: We need to find possible lookahead strings!
			State ns(RuleIter(*rule), ptr.step);
			s.insert(ns);
		}
	}

	void Parser::scanner(StateSet &s, State state, StatePtr ptr) {
		SyntaxToken *token = state.pos.token();
		RegexToken *t = dynamic_cast<RegexToken*>(token);
		if (!t)
			return;

		nat matched = t->regex.match(src, ptr.step);
		if (matched == NO_MATCH)
			return;

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

		String completed = state.pos.rule().type();
		StateSet &from = steps[state.from];
		for (nat i = 0; i < from.size(); i++) {
			State st = from[i];
			StatePtr stPtr(state.from, i);
			if (!st.isRule(completed))
				continue;

			State ns(st.pos.nextA(), st.from, stPtr, ptr);
			s.insert(ns);

			ns.pos = st.pos.nextB();
			s.insert(ns);
		}
	}

	bool Parser::hasError() const {
		if (lastStep() < steps.size() - 1) {
			return true;
		}

		const StateSet &s = steps.back();
		for (nat i = 0; i < s.size(); i++)
			if (s[i].finish(&rootRule))
				return false;

		return true;
	}

	SyntaxError Parser::error(const Path &path) const {
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

		return SyntaxError(SrcPos(path, pos), oss.str());
	}

	nat Parser::lastStep() const {
		for (nat i = steps.size() - 1; i > 0; i--) {
			if (steps[i].size() != 0)
				return i;
		}
		return 0;
	}

	Parser::StatePtr Parser::finish() const {
		for (nat i = steps.size() - 1; i > 0; i--) {
			for (nat j = 0; j < steps[i].size(); j++) {
				if (steps[i][j].finish(&rootRule))
					return StatePtr(i, j);
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
				r.insert(states[i].tokenRegex().toS());
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
		SyntaxVariable *resultVar = root->find(L"root", SyntaxVariable::tNode);
		if (resultVar) {
			result = resultVar->node();
			resultVar->orphan();
		}
		delete root;

		return result;
	}

	SyntaxNode *Parser::tree(StatePtr pos) {
		State *cState = &state(pos);
		SyntaxNode *result = null;
		SyntaxNode *tmp = null;

		try {
			result = new SyntaxNode(&cState->pos.rule());

			for (; cState->prev.valid(); pos = cState->prev, cState = &state(pos)) {
				State *pState = &state(cState->prev);

				if (pState->bindToken()) {
					const String &to = pState->bindTokenTo();

					if (cState->completed.valid()) {
						tmp = tree(cState->completed);
						result->add(to, tmp);
						tmp = null;
					} else {
						nat fromPos = cState->prev.step;
						nat toPos = pos.step;
						String matched = src.substr(fromPos, toPos - fromPos);
						result->add(to, matched);
					}
				}
			}

			result->reverseArrays();
		} catch (...) {
			delete result;
			delete tmp;
			throw;
		}

		return result;
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

	bool Parser::State::finish(const SyntaxRule *rootRule) const {
		return &pos.rule() == rootRule
			&& pos.end();
		// We could check pos.from here as well, but we can never instantiate that rule more than once!
	}

	/**
	 * State set.
	 */

	void Parser::StateSet::insert(const State &state) {
		if (!state.pos.valid())
			return;

		for (nat i = 0; i < size(); i++)
			if ((*this)[i] == state)
				return;
		push_back(state);
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
