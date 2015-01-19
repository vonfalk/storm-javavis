#include "stdafx.h"
#include "BSClass.h"
#include "SyntaxSet.h"
#include "Parser.h"
#include "BSScope.h"
#include "BSFunction.h"
#include "TypeCtor.h"

namespace storm {

	bs::Class::Class(SrcPos pos, Par<SStr> name, Par<SStr> content)
		: Type(name->v->v, typeClass), declared(pos), content(content) {}

	void bs::Class::setScope(const Scope &scope) {
		this->scope = Scope(scope, this);
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

		for (nat i = 0; i < body->items.size(); i++) {
			add(body->items[i].borrow());
		}

		// Temporary solution.
		add(steal(CREATE(TypeDefaultCtor, engine, this)));
	}

	/**
	 * Body
	 */

	bs::ClassBody::ClassBody() {}

	void bs::ClassBody::add(Par<NameOverload> i) {
		items.push_back(i);
	}

	/**
	 * Member
	 */

	bs::ClassVar::ClassVar(Par<Class> owner, Par<TypeName> type, Par<SStr> name)
		: TypeVar(owner.borrow(), type->value(owner->scope), name->v->v) {}


	bs::BSFunction *STORM_FN bs::classFn(Par<Class> owner,
										SrcPos pos,
										Par<SStr> name,
										Par<TypeName> result,
										Par<Params> params,
										Par<SStr> contents) {

		params->addThis(owner.borrow());
		return CREATE(BSFunction, owner->engine,
					result->value(owner->scope),
					name->v->v,
					params->cTypes(owner->scope),
					params->cNames(),
					owner->scope,
					contents,
					pos);
	}

}
