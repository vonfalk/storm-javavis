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

	bs::Class *bs::createClass(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<syntax::Node> body) {
		return CREATE(Class, name, typeClass, pos, env->scope, name->v->v, body);
	}

	bs::Class *bs::createValue(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<syntax::Node> body) {
		return CREATE(Class, name, typeValue, pos, env->scope, name->v->v, body);
	}

	bs::Class *bs::extendClass(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<Name> from, Par<syntax::Node> body) {
		Class *c = CREATE(Class, name, typeClass, pos, env->scope, name->v->v, body);
		c->base = from;
		return c;
	}

	bs::Class *bs::extendValue(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<Name> from, Par<syntax::Node> body) {
		Class *c = CREATE(Class, name, typeValue, pos, env->scope, name->v->v, body);
		c->base = from;
		return c;
	}

	bs::Class *bs::threadClass(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<Name> thread, Par<syntax::Node> body) {
		Class *c = CREATE(Class, name, typeClass, pos, env->scope, name->v->v, body);
		c->thread = thread;
		return c;
	}

	bs::Class *bs::threadClass(SrcPos pos, Par<SyntaxEnv> env, Par<SStr> name, Par<syntax::Node> body) {
		Class *c = CREATE(Class, name, typeClass, pos, env->scope, name->v->v, body);
		c->setSuper(TObject::stormType(c->engine));
		return c;
	}


	bs::Class::Class(TypeFlags flags, const SrcPos &pos, const Scope &scope, const String &name, Par<syntax::Node> body)
		: Type(name, flags), scope(scope, this), declared(pos), body(body), allowLazyLoad(true) {}


	void bs::Class::lookupTypes() {
		allowLazyLoad = false;
		try {

			if (base) {
				Auto<Named> n = scope.find(base);
				Type *b = as<Type>(n.borrow());
				if (!b)
					throw SyntaxError(declared, L"Base class " + ::toS(base) + L" undefined!");

				setSuper(b);
				base = null;
			}

			if (thread) {
				Auto<NamedThread> t = steal(scope.find(thread)).as<NamedThread>();
				if (!t)
					throw SyntaxError(declared, L"Can not find the thread " + ::toS(name) + L".");

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

		Auto<ClassBody> body = syntax::transformNode<ClassBody, Class>(this->body, this);

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

		for (MAP_PP(Str, TemplateAdapter)::Iter i = body->templates->begin(), end = body->templates->end(); i != end; ++i) {
			add(i.val());
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

	void bs::ClassBody::add(Par<TObject> o) {
		if (Named *n = as<Named>(o.borrow())) {
			add(Par<Named>(n));
		} else if (Template *t = as<Template>(o.borrow())) {
			add(Par<Template>(t));
		} else {
			throw InternalError(L"Not a suitable type to ClassBody.add(): " + o->myType->identifier());
		}
	}

	/**
	 * Member
	 */

	bs::ClassVar::ClassVar(Par<Class> owner, Par<SrcName> type, Par<SStr> name)
		: TypeVar(owner.borrow(), owner->scope.value(type), name->v->v) {}


	bs::BSFunction *STORM_FN bs::classFn(Par<Class> owner,
										SrcPos pos,
										Par<SStr> name,
										Par<Name> result,
										Par<Params> params,
										Par<syntax::Node> body) {

		params->addThis(owner.borrow());
		return CREATE(BSFunction, owner->engine,
					owner->scope.value(result, pos),
					name,
					params,
					owner->scope,
					null, // thread
					body);
	}

	bs::BSCtor *STORM_FN bs::classCtor(Par<Class> owner,
										SrcPos pos,
										Par<Params> params,
										Par<syntax::Node> body) {
		params->addThis(owner.borrow());
		vector<String> names = params->cNames();
		vector<Value> values = params->cTypes(owner->scope);

		return CREATE(BSCtor, owner->engine, values, names, owner->scope, body, pos);
	}

	bs::BSCtor *STORM_FN bs::classCastCtor(Par<Class> owner,
										SrcPos pos,
										Par<Params> params,
										Par<syntax::Node> body) {
		BSCtor *r = classCtor(owner, pos, params, body);
		r->flags |= namedAutoCast;
		return r;
	}


	bs::BSCtor *STORM_FN bs::classDefaultCtor(Par<Class> owner) {
		Auto<Params> params = CREATE(Params, owner);

		return classCtor(owner, owner->declared, params, null);
	}


}
