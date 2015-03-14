#include "stdafx.h"
#include "SyntaxRule.h"
#include "Exception.h"

namespace storm {

	SyntaxRule::SyntaxRule(const String &name, const Scope &scope, bool owner)
		: rName(name), owner(owner), declared(), declScope(scope) {}

	SyntaxRule::~SyntaxRule() {
		if (owner) {
			clear(options);
		}
	}

	void SyntaxRule::orphanOptions() {
		options.clear();
	}

	void SyntaxRule::copyDeclaration(const SyntaxRule &o) {
		declared = o.declared;
		params = o.params;
		declScope = o.declScope;
	}

	void SyntaxRule::add(SyntaxOption *rule) {
		options.push_back(rule);
	}

	void SyntaxRule::output(std::wostream &to) const {
		to << rName << L"(";
		join(to, params, L", ");
		to << L")" << endl;

		for (nat i = 0; i < options.size(); i++) {
			to << *options[i] << endl;
		}
	}

	wostream &operator <<(wostream &to, const SyntaxRule::Param &p) {
		return to << p.type << L" " << p.name;
	}


	/**
	 * Collection.
	 */

	SyntaxRules::~SyntaxRules() {
		clear();
	}

	void SyntaxRules::clear() {
		clearMap(data);
	}

	SyntaxRule *SyntaxRules::operator [](const String &name) const {
		Map::const_iterator i = data.find(name);
		if (i == data.end())
			return null;
		return i->second;
	}

	void SyntaxRules::add(SyntaxRule *rule) {
		Map::iterator i = data.find(rule->name());
		if (i == data.end()) {
			data.insert(make_pair(rule->name(), rule));
		} else {
			try {
				merge(i->second, rule);
			} catch (...) {
				delete rule;
			}
			delete rule;
		}
	}

	void SyntaxRules::merge(SyntaxRule *to, SyntaxRule *from) {
		if (to->declared.unknown()) {
			to->copyDeclaration(*from);
		} else if (!from->declared.unknown()) {
			throw SyntaxError(from->declared, L"The rule " + to->name() +
							L" was already declared at " + ::toS(to->declared));
		}

		for (nat i = 0; i < from->size(); i++)
			to->add((*from)[i]);

		from->orphanOptions();
	}

	void SyntaxRules::output(wostream &to) const {
		for (Map::const_iterator i = data.begin(); i != data.end(); i++) {
			to << *i->second << endl;
		}
	}

}
