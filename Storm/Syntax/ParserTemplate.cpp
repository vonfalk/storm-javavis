#include "stdafx.h"
#include "ParserTemplate.h"
#include "Parser.h"
#include "Engine.h"

namespace storm {
	namespace syntax {

		static void CODECALL createParser(void *mem) {
			new (mem) ParserBase();
			setVTable((Object *)mem);
		}

		static void CODECALL createParserPkg(void *mem, Package *p) {
			createParser(mem);
			ParserBase *b = (ParserBase *)mem;
			b->addSyntax(p);
		}

		static void CODECALL createParserArr(void *mem, ArrayP<Package> *p) {
			createParser(mem);
			ParserBase *b = (ParserBase *)mem;
			b->addSyntax(p);
		}

		static void CODECALL destroyParser(ParserBase *v) {
			v->~ParserBase();
		}

		static Object *parserTree(ParserBase *me) {
			return me->tree();
		}

		static Named *generateParser(Par<NamePart> part) {
			if (part->params.size() != 1)
				return null;

			Type *param = part->params[0].type;
			if (Rule *root = as<Rule>(param)) {
				return CREATE(ParserType, root, root);
			}

			return null;
		}

		void addParserTemplate(Engine &to) {
			// TODO: New package later! See Parser.h.
			Package *pkg = to.package(L"lang.syntax", true);

			Auto<Template> t = CREATE(Template, to, L"Parser", simpleFn(&generateParser));
			pkg->add(t);
		}

		ParserType::ParserType(Par<Rule> root) :
			Type(L"Parser", typeClass, valList(1, Value::thisPtr(root))),
			rootRule(root.borrow()) {

			setSuper(ParserBase::stormType(engine));
		}

		Rule *ParserType::root() {
			rootRule->addRef();
			return rootRule;
		}

		bool ParserType::loadAll() {
			Engine &e = engine;
			Value t = Value::thisPtr(this);
			Value pkg = Value::thisPtr(Package::stormType(e));
			Value arr = Value::thisPtr(arrayType(e, pkg));
			Value rule = Value::thisPtr(rootRule);

			add(steal(nativeFunction(e, Value(), Type::CTOR, valList(1, t), address(&createParser))));
			add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, pkg), address(&createParserPkg))));
			add(steal(nativeFunction(e, Value(), Type::CTOR, valList(2, t, arr), address(&createParserArr))));
			add(steal(nativeFunction(e, rule, L"tree", valList(1, t), address(&parserTree))));
			add(steal(nativeDtor(e, this, &destroyParser)));

			return Type::loadAll();
		}

	}
}
