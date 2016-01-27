#include "stdafx.h"
#include "BSClass.h"
#include "SyntaxSet.h"
#include "Parser.h"
#include "BSScope.h"
#include "BSFunction.h"
#include "BSCtor.h"
#include "TypeCtor.h"
#include "TypeDtor.h"
#include "TypeToS.h"
#include "Shared/CloneEnv.h"
#include "Shared/TObject.h"

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

	bs::Class *bs::threadClass(SrcPos pos, Par<SStr> name, Par<SStr> content) {
		Class *c = CREATE(Class, name, typeClass, pos, name->v->v, content);
		c->setSuper(TObject::stormType(c->engine));
		return c;
	}


	bs::Class::Class(TypeFlags flags, const SrcPos &pos, const String &name, Par<SStr> content)
		: Type(name, flags), declared(pos), content(content), allowLazyLoad(true) {}


	void bs::Class::setScope(const Scope &scope) {
		this->scope = Scope(scope, this);
	}

	void bs::Class::lookupTypes() {
		allowLazyLoad = false;
		try {

			if (base) {
				Value t = base->resolve(scope);
				setSuper(t.type);
				base = null;
			}

			if (thread) {
				Auto<Name> name = thread->toName(scope);
				Auto<NamedThread> t = steal(scope.find(name)).as<NamedThread>();
				if (!t)
					throw SyntaxError(thread->pos, L"Can not find the thread " + ::toS(name) + L".");

				setThread(t);
				thread = null;
			}

		} catch (...) {
			allowLazyLoad = true;
			throw;
		}
		allowLazyLoad = true;
	}

	bool bs::Class::loadAll() {
		if (!allowLazyLoad)
			return false;

		Auto<SyntaxSet> syntax = getSyntax(scope);

		Auto<Parser> parser = CREATE(Parser, this, syntax, content->v, content->pos);
		parser->parse(L"ClassBody");
		if (parser->hasError())
			throw parser->error();

		Auto<Object> z = parser->transform(vector<Object *>(1, this));
		Auto<ClassBody> body = z.expect<ClassBody>(engine, L"From ClassBody rule");

		// Found a CTOR?
		bool hasCtor = false;
		bool hasCopyCtor = false;
		bool hasDeepCopy = false;

		for (nat i = 0; i < body->items->count(); i++) {
			Auto<Named> &z = body->items->at(i);
			if (z->name == Type::CTOR) {
				if (z->params.size() == 2 && z->params[0] == z->params[1])
					hasCopyCtor = true;
				hasCtor = true;
			} else if (z->name == L"deepCopy") {
				if (z->params.size() == 2 && z->params[1].type == CloneEnv::stormType(this))
					hasDeepCopy = true;
			}
			add(z.borrow());
		}

		add(steal(CREATE(TypeDefaultDtor, engine, this)));
		if (!hasCtor)
			add(steal(classDefaultCtor(this)));
		if (!hasCopyCtor && runOn().state == RunOn::any)
			add(steal(CREATE(TypeCopyCtor, engine, this)));

		if (!hasDeepCopy && runOn().state == RunOn::any)
			add(steal(CREATE(TypeDeepCopy, engine, this)));

		// If noone has a toS function, create one (this is the case with values, this may change in the future).
		Auto<Named> toS = findCpp(L"toS", valList(1, Value::thisPtr(this)));
		if (!toS.as<Function>()) {
			add(steal(CREATE(TypeToS, engine, this)));
		}

		// Temporary solution.
		if (typeFlags & typeValue) {
			add(steal(CREATE(TypeAssignFn, engine, this)));
		}

		// Super needs to be called!
		return Type::loadAll();
	}

	/**
	 * Body
	 */

	bs::ClassBody::ClassBody() :
		items(CREATE(ArrayP<Named>, this)),
		templates(CREATE(MAP_PP(Str, TemplateAdapter), this)) {}

	void bs::ClassBody::add(Par<Named> i) {
		items->push(i);
	}

	void bs::ClassBody::add(Par<Template> t) {
		Auto<Str> k = CREATE(Str, this, t->name);
		if (templates->has(k)) {
			Auto<TemplateAdapter> &found = templates->get(k);
			found->add(t);
		} else {
			Auto<TemplateAdapter> adapter = CREATE(TemplateAdapter, this, k);
			adapter->add(t);
			templates->put(k, adapter);
		}
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

	bs::BSCtor *STORM_FN bs::classCastCtor(Par<Class> owner,
										SrcPos pos,
										Par<Params> params,
										Par<SStr> contents) {
		BSCtor *r = classCtor(owner, pos, params, contents);
		r->flags |= namedAutoCast;
		return r;
	}


	bs::BSCtor *STORM_FN bs::classDefaultCtor(Par<Class> owner) {
		Auto<Params> params = CREATE(Params, owner);

		return classCtor(owner, owner->declared, params, null);
	}


}
