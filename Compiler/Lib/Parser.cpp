#include "stdafx.h"
#include "Parser.h"
#include "TemplateList.h"
#include "Exception.h"
#include "Engine.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Parser.h"

namespace storm {
	using syntax::Rule;

	Type *createParser(Str *name, ValueArray *params) {
		if (params->count() != 1)
			return null;

		Value param = params->at(0);
		if (param.ref)
			return null;

		if (Rule *r = as<Rule>(param.type))
			return new (params) ParserType(name, r);
		else
			return null;
	}

	ParserType *parserType(Rule *rule) {
		Engine &e = rule->engine;
		TemplateList *l = e.cppTemplate(syntax::ParserId);
		NameSet *to = l->addTo();
		assert(to, L"Too early to use 'parserType'");
		ParserType *found = as<ParserType>(to->find(new (e) SimplePart(new (e) Str(L"Parser"), Value(rule))));
		if (!found)
			throw InternalError(L"Can not find the parser type!");
		return found;
	}

	ParserType::ParserType(Str *name, Rule *rule)
		: Type(name, new (this) Array<Value>(1, Value(rule)), typeClass),
		  root(rule) {

		setSuper(syntax::ParserBase::stormType(engine));
	}

	Bool ParserType::loadAll() {
		// TODO: Load additional members!

		return Type::loadAll();
	}

}
