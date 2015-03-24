#include "stdafx.h"
#include "BSClass.h"
#include "SyntaxSet.h"
#include "Parser.h"
#include "BSScope.h"
#include "BSFunction.h"
#include "BSCtor.h"
#include "TypeCtor.h"
#include "TypeDtor.h"
#include "Lib/CloneEnv.h"

namespace storm {

	bs::Class *bs::createClass(SrcPos pos, Par<SStr> name, Par<SStr> content) {
		return CREATE(Class, name, typeClass, pos, name->v->v, content);
	}

	bs::Class *bs::createValue(SrcPos pos, Par<SStr> name, Par<SStr> content) {
		return CREATE(Class, name, typeValue, pos, name->v->v, content);
	}

	bs::Class *bs::extendClass(SrcPos pos, Par<SStr> name, Par<TypeName> from, Par<SStr> content) {
		Class *c = CREATE(Class, name, typeClass, pos, name->v->v, content);
		c->base = from;
		return c;
	}

	bs::Class *bs::extendValue(SrcPos pos, Par<SStr> name, Par<TypeName> from, Par<SStr> content) {
		Class *c = CREATE(Class, name, typeValue, pos, name->v->v, content);
		c->base = from;
		return c;
	}

	bs::Class *bs::threadClass(SrcPos pos, Par<SStr> name, Par<TypeName> thread, Par<SStr> content) {
		Class *c = CREATE(Class, name, typeClass, pos, name->v->v, content);
		c->thread = thread;
		return c;
	}


	bs::Class::Class(TypeFlags flags, const SrcPos &pos, const String &name, Par<SStr> content)
		: Type(name, flags), declared(pos), content(content) {}


	void bs::Class::setScope(const Scope &scope) {
		this->scope = Scope(scope, this);
	}

	void bs::Class::lookupTypes() {
		allowLazyLoad(false);

		if (base) {
			Value t = base->resolve(scope);
			setSuper(t.type);
			base = null;
		}

		if (thread) {
			Auto<Name> name = thread->toName(scope);
			NamedThread *t = as<NamedThread>(scope.find(name));
			if (!t)
				throw SyntaxError(thread->pos, L"Can not find the thread " + ::toS(name) + L".");

			setThread(t);
			thread = null;
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
		bool hasCopyCtor = false;
		bool hasDeepCopy = false;

		for (nat i = 0; i < body->items.size(); i++) {
			Auto<Named> z = body->items[i];
			if (z->name == Type::CTOR) {
				if (z->params.size() == 2 && z->params[0] == z->params[1])
					hasCopyCtor = true;
				hasCtor = true;
			} else if (z->name == L"deepCopy") {
				if (z->params.size() == 2 && z->params[1].type == CloneEnv::type(this))
					hasDeepCopy = true;
			}
			add(z.borrow());
		}

		add(steal(CREATE(TypeDefaultDtor, engine, this)));
		if (!hasCtor)
			add(steal(classDefaultCtor(this)));
		if (!hasCopyCtor)
			add(steal(CREATE(TypeCopyCtor, engine, this)));

		if (!hasDeepCopy)
			add(steal(CREATE(TypeDeepCopy, engine, this)));

		// Temporary solution.
		if (flags & typeValue) {
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
					null, // thread
					pos,
					true);
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

	bs::BSCtor *STORM_FN bs::classDefaultCtor(Par<Class> owner) {
		Auto<Params> params = CREATE(Params, owner);

		return classCtor(owner, owner->declared, params, null);
	}


}
