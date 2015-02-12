#include "stdafx.h"
#include "BSClass.h"
#include "SyntaxSet.h"
#include "Parser.h"
#include "BSScope.h"
#include "BSFunction.h"
#include "BSCtor.h"
#include "TypeCtor.h"
#include "TypeDtor.h"

namespace storm {

	static TypeFlags flag(Par<SStr> type) {
		if (type->v->v == L"class")
			return typeClass;
		else if (type->v->v == L"value")
			return typeValue;
		else
			throw SyntaxError(type->pos, L"Expected class or value.");
	}

	bs::Class::Class(SrcPos pos, Par<SStr> t, Par<SStr> name, Par<SStr> content)
		: Type(name->v->v, flag(t)), declared(pos), content(content) {}

	bs::Class::Class(SrcPos pos, Par<SStr> t, Par<SStr> name, Par<SStr> content, Par<TypeName> base)
		: Type(name->v->v, flag(t)), declared(pos), content(content), base(base) {}

	void bs::Class::setScope(const Scope &scope) {
		this->scope = Scope(scope, this);
	}

	void bs::Class::setBase() {
		allowLazyLoad(false);
		if (base) {
			Value t = base->resolve(scope);
			setSuper(t.type);
			base = null;
		}
		allowLazyLoad(true);
	}

	void bs::Class::lazyLoad() {
		SyntaxSet syntax;
		addSyntax(scope, syntax);

		Parser parser(syntax, content->v->v, content->pos);
		parser.parse(L"ClassBody");
		if (parser.hasError())
			throw parser.error();

		Auto<Object> z = parser.transform(engine, vector<Object *>(1, this));
		Auto<ClassBody> body = z.expect<ClassBody>(engine, L"From ClassBody rule");

		// Found a CTOR?
		bool hasCtor = false;

		for (nat i = 0; i < body->items.size(); i++) {
			Auto<Named> z = body->items[i];
			if (z->name == Type::CTOR)
				hasCtor = true;
			add(z.borrow());
		}

		add(steal(CREATE(TypeDefaultDtor, engine, this)));
		if (!hasCtor)
			add(steal(CREATE(TypeDefaultCtor, engine, this)));

		// Temporary solution.
		if (flags & typeValue) {
			add(steal(CREATE(TypeCopyCtor, engine, this)));
			add(steal(CREATE(TypeAssignFn, engine, this)));
		}
	}

	/**
	 * Body
	 */

	bs::ClassBody::ClassBody() {}

	void bs::ClassBody::add(Par<Named> i) {
		items.push_back(i);
	}

	/**
	 * Member
	 */

	bs::ClassVar::ClassVar(Par<Class> owner, Par<TypeName> type, Par<SStr> name)
		: TypeVar(owner.borrow(), type->resolve(owner->scope), name->v->v) {}


	bs::BSFunction *STORM_FN bs::classFn(Par<Class> owner,
										SrcPos pos,
										Par<SStr> name,
										Par<TypeName> result,
										Par<Params> params,
										Par<SStr> contents) {

		params->addThis(owner.borrow());
		return CREATE(BSFunction, owner->engine,
					result->resolve(owner->scope),
					name->v->v,
					params->cTypes(owner->scope),
					params->cNames(),
					owner->scope,
					contents,
					pos);
	}

	bs::BSCtor *STORM_FN bs::classCtor(Par<Class> owner,
										SrcPos pos,
										Par<Params> params,
										Par<SStr> contents) {
		params->addThis(owner.borrow());
		vector<String> names = params->cNames();
		vector<Value> values = params->cTypes(owner->scope);

		return CREATE(BSCtor, owner->engine, values, names, owner->scope, contents, pos);
	}

}
