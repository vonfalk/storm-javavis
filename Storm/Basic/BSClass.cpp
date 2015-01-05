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

		Parser parser(syntax, content->v->v, content->pos.file());
		parser.parse(L"ClassBody", content->pos.offset);
		if (parser.hasError())
			throw parser.error();

		Engine &e = Object::engine();
		Auto<ClassBody> body = Auto<Object>(parser.transform(e)).expect<ClassBody>(e, L"From ClassBody rule");
	}

	/**
	 * Body
	 */

	bs::ClassBody::ClassBody() {}

	void bs::ClassBody::add(Auto<Named> i) {
		items.push_back(i);
	}

}
