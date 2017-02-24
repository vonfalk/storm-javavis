#include "stdafx.h"
#include "Parser.h"
#include "TemplateList.h"
#include "Exception.h"
#include "Engine.h"
#include "Array.h"
#include "Compiler/Syntax/Rule.h"
#include "Compiler/Syntax/Parser.h"
#include "Compiler/Package.h"

namespace storm {
	using syntax::Rule;
	using syntax::ParserBase;
	using syntax::ParserBackend;

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

	namespace syntax {
		static void CODECALL createDefParser(void *mem) {
			createParser(mem, null);
		}

		static void CODECALL createParser(void *mem, ParserBackend *backend) {
			ParserBase *p = new (Place(mem)) ParserBase(backend);
			runtime::setVTable(p);
		}
	}

	static void CODECALL createParserPkg(void *mem, Package *p, ParserBackend *backend) {
		syntax::createParser(mem, backend);
		((ParserBase *)mem)->addSyntax(p);
	}

	static void CODECALL createDefParserPkg(void *mem, Package *p) {
		createParserPkg(mem, p, null);
	}

	static void CODECALL createParserArr(void *mem, Array<Package *> *p, ParserBackend *backend) {
		syntax::createParser(mem, backend);
		((ParserBase *)mem)->addSyntax(p);
	}

	static void CODECALL createDefParserArr(void *mem, Array<Package *> *p) {
		createParserArr(mem, p, null);
	}

	static TObject *CODECALL parserTree(ParserBase *me) {
		return me->tree();
	}

	ParserType::ParserType(Str *name, Rule *rule)
		: Type(name, new (this) Array<Value>(1, Value(rule)), typeClass),
		  root(rule) {

		setSuper(syntax::ParserBase::stormType(engine));
	}

	Bool ParserType::loadAll() {
		Engine &e = engine;
		Value t = thisPtr(this);
		Value pkg = thisPtr(Package::stormType(e));
		Value arr = wrapArray(pkg);
		Value rule = thisPtr(root);
		Value backend = thisPtr(ParserBackend::stormType(e));

		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 1, t), address(&syntax::createDefParser)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, backend), address(&syntax::createParser)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, pkg), address(&createDefParserPkg)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 3, t, pkg, backend), address(&createParserPkg)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 2, t, arr), address(&createDefParserArr)));
		add(nativeFunction(e, Value(), Type::CTOR, valList(e, 3, t, arr, backend), address(&createParserArr)));
		add(nativeFunction(e, rule, L"tree", valList(e, 1, t), address(&parserTree)));

		return Type::loadAll();
	}

}
