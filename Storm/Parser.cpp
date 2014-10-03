#include "stdafx.h"
#include "Parser.h"

#include "Package.h"

namespace storm {

	void SyntaxSet::add(Package &pkg) {
		assert(syntax.size() == 0); // No support for merging yet!
		syntax = pkg.syntax();
	}

	nat SyntaxSet::parse(const String &root, const String &src, nat start) {
		Parser p(*this, src);
		nat r = p.parse(root, start);
		if (p.hasError()) {
			PLN(p.error(Path()));
		}
		return r;
	}

	/**
	 * Parser implementation.
	 */

	Parser::Parser(SyntaxSet &set, const String &src) : syntax(set), src(src) {}

	nat Parser::parse(const String &rootType, nat pos) {
		rootRule.clear();
		rootRule.add(new TypeToken(rootType));

		steps = vector<StateSet>(src.size() + 1);
		steps[pos].insert(State(rootRule, 0, L""));

		nat len = 0;

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
			predictor(s, step, s[i]);
			completer(s, step, s[i]);
			scanner(s, step, s[i]);

			if (s[i].finish(&rootRule))
				seenFinish = true;
		}

		return seenFinish;
	}

	void Parser::predictor(StateSet &s, nat pos, State state) {
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
			State ns(RuleIter(*rule), pos, L"");
			s.insert(ns);
		}
	}

	void Parser::scanner(StateSet &s, nat pos, State state) {
		SyntaxToken *token = state.pos.token();
		RegexToken *t = dynamic_cast<RegexToken*>(token);
		if (!t)
			return;

		nat matched = t->regex.match(src, pos);
		if (matched == NO_MATCH)
			return;

		// Should not happen, but best to be safe!
		if (matched >= steps.size())
			return;

		State ns(state);
		ns.pos = state.pos.nextA();
		steps[matched].insert(ns);

		ns.pos = state.pos.nextB();
		steps[matched].insert(ns);
	}

	void Parser::completer(StateSet &s, nat pos, State state) {
		if (!state.pos.end())
			return;

		String completed = state.pos.rule().type();
		StateSet &from = steps[state.from];
		for (nat i = 0; i < from.size(); i++) {
			State st = from[i];
			if (!st.isRule(completed))
				continue;

			st.pos = from[i].pos.nextA();
			s.insert(st);

			st.pos = from[i].pos.nextB();
			s.insert(st);
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


	/**
	 * State.
	 */
	void Parser::State::output(wostream &to) const {
		to << "{State: " << pos << ", " << from << ", ";
		if (lookahead.empty())
			to << L"<?>";
		else
			to << lookahead;
		to << "}";
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

	bool Parser::State::finish(const SyntaxRule *rootRule) const {
		return &pos.rule() == rootRule
			&& pos.end();
		// We could check pos.from here as well, but we can never instantiate that rule again!
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
}
