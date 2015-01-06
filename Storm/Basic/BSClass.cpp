#include "stdafx.h"
#include "BSClass.h"
#include "BSScope.h"
#include "SyntaxSet.h"
#include "Parser.h"

namespace storm {

	bs::Class::Class(SrcPos pos, Auto<SStr> name, Auto<SStr> content)
		: Type(name->v->v, typeClass), declared(pos), content(content) {}

	void bs::Class::setScope(const Scope &scope) {
		this->scope = Scope(scope, capture(this));
	}

	void bs::Class::lazyLoad() {
		SyntaxSet syntax;
		addSyntax(scope, syntax);

		Parser parser(syntax, content->v->v, content->pos);
		parser.parse(L"ClassBody");
		if (parser.hasError())
			throw parser.error();

		Engine &e = Object::engine();
		Auto<Object> z = parser.transform(e, vector<Object *>(1, this));
		Auto<ClassBody> body = z.expect<ClassBody>(e, L"From ClassBody rule");

		for (nat i = 0; i < body->items.size(); i++) {
			add(body->items[i].steal());
		}
	}

	/**
	 * Body
	 */

	bs::ClassBody::ClassBody() {}

	void bs::ClassBody::add(Auto<NameOverload> i) {
		items.push_back(i);
	}

	/**
	 * Member
	 */

	bs::Variable::Variable(Auto<Class> owner, Auto<TypeName> type, Auto<SStr> name)
		: TypeVar(owner.borrow(), type->value(owner->scope), name->v->v) {}

}
