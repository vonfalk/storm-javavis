#include "stdafx.h"
#include "Class.h"
#include "Compiler/Exception.h"
#include "Function.h"
#include "Ctor.h"

namespace storm {
	namespace bs {

		Class *createClass(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body) {
			return new (name) Class(typeClass, pos, env, name->v, body);
		}

		Class *createValue(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body) {
			return new (name) Class(typeValue, pos, env, name->v, body);
		}

		Class *extendClass(SrcPos pos, Scope env, syntax::SStr *name, Name *from, syntax::Node *body) {
			Class *c = new (name) Class(typeClass, pos, env, name->v, body);
			c->base = from;
			return c;
		}

		Class *extendValue(SrcPos pos, Scope env, syntax::SStr *name, Name *from, syntax::Node *body) {
			Class *c = new (name) Class(typeValue, pos, env, name->v, body);
			c->base = from;
			return c;
		}

		Class *threadClass(SrcPos pos, Scope env, syntax::SStr *name, Name *thread, syntax::Node *body) {
			Class *c = new (name) Class(typeClass, pos, env, name->v, body);
			c->thread = thread;
			return c;
		}

		Class *threadClass(SrcPos pos, Scope env, syntax::SStr *name, syntax::Node *body) {
			Class *c = new (name) Class(typeClass, pos, env, name->v, body);
			c->setSuper(TObject::stormType(c->engine));
			return c;
		}


		Class::Class(TypeFlags flags, SrcPos pos, Scope scope, Str *name, syntax::Node *body)
			: Type(name, flags), scope(scope, this), declared(pos), body(body), allowLazyLoad(true) {}


		void Class::lookupTypes() {
			allowLazyLoad = false;
			try {

				if (base) {
					Named *n = scope.find(base);
					Type *b = as<Type>(n);
					if (!b)
						throw SyntaxError(declared, L"Base class " + ::toS(base) + L" undefined!");

					setSuper(b);
					base = null;
				}

				if (thread) {
					NamedThread *t = as<NamedThread>(scope.find(thread));
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

		bool Class::loadAll() {
			if (!allowLazyLoad)
				return false;

			TODO(L"Implement me!");
			// ClassBody *body = syntax::transformNode<ClassBody, Class>(this->body, this);

			// // Found a CTOR?
			// bool hasCtor = false;
			// bool hasCopyCtor = false;
			// bool hasDeepCopy = false;

			// for (nat i = 0; i < body->items->count(); i++) {
			// 	Named *&z = body->items->at(i);
			// 	if (z->name == Type::CTOR) {
			// 		if (z->params->count() == 2 && z->params[0] == z->params[1])
			// 			hasCopyCtor = true;
			// 		hasCtor = true;
			// 	} else if (z->name == L"deepCopy") {
			// 		if (z->params->count() == 2 && z->params[1].type == CloneEnv::stormType(this))
			// 			hasDeepCopy = true;
			// 	}
			// 	add(z);
			// }

			// add(steal(new (engine) TypeDefaultDtor(this)));
			// if (!hasCtor)
			// 	add(steal(classDefaultCtor(this)));
			// if (!hasCopyCtor && runOn().state == RunOn::any)
			// 	add(steal(new (engine) TypeCopyCtor(this)));

			// if (!hasDeepCopy && runOn().state == RunOn::any)
			// 	add(steal(new (engine) TypeDeepCopy(this)));

			// // If noone has a toS function, create one (this is the case with values, this may change in the future).
			// Named *toS = findCpp(L"toS", valList(1, Value::thisPtr(this)));
			// if (!as<Function>(toS)) {
			// 	add(steal(new (engine) TypeToS(this)));
			// }

			// // Temporary solution.
			// if (typeFlags & typeValue) {
			// 	add(steal(new (engine) TypeAssignFn(this)));
			// }

			// for (Map<Str *, TemplateAdapter *>::Iter i = body->templates->begin(), end = body->templates->end(); i != end; ++i) {
			// 	add(i.val());
			// }

			// // We do not need the syntax tree anymore!
			// this->body = null;

			// Super needs to be called!
			return Type::loadAll();
		}

		/**
		 * Body
		 */

		ClassBody::ClassBody() {
			items = new (this) Array<Named *>();
			templates = new (this) Array<Template *>();
		}

		void ClassBody::add(Named *i) {
			items->push(i);
		}

		void ClassBody::add(Template *t) {
			templates->push(t);
		}

		void ClassBody::add(TObject *o) {
			if (Named *n = as<Named>(o)) {
				add(n);
			} else if (Template *t = as<Template>(o)) {
				add(t);
			} else {
				throw InternalError(L"Not a suitable type to ClassBody.add(): " + ::toS(runtime::typeOf(o)->identifier()));
			}
		}

		/**
		 * Member
		 */

		ClassVar::ClassVar(Class *owner, SrcName *type, syntax::SStr *name)
			: MemberVar(name->v, owner->scope.value(type), owner) {}


		BSFunction *STORM_FN classFn(Class *owner,
											SrcPos pos,
											syntax::SStr *name,
											Name *result,
											Array<NameParam> *params,
											syntax::Node *body) {

			return new (owner) BSFunction(owner->scope.value(result, pos),
										name,
										resolve(params, owner, owner->scope),
										owner->scope,
										null, // thread
										body);
		}

		BSCtor *STORM_FN classCtor(Class *owner,
										SrcPos pos,
										Array<NameParam> *params,
										syntax::Node *body) {

			return new (owner) BSCtor(resolve(params, owner, owner->scope),
									owner->scope,
									body,
									pos);
		}

		BSCtor *STORM_FN classCastCtor(Class *owner,
											SrcPos pos,
											Array<NameParam> *params,
											syntax::Node *body) {
			BSCtor *r = classCtor(owner, pos, params, body);
			r->flags |= namedAutoCast;
			return r;
		}


		BSCtor *STORM_FN classDefaultCtor(Class *owner) {
			Array<NameParam> *params = CREATE(Array<NameParam>, owner);

			return classCtor(owner, owner->declared, params, null);
		}

	}
}
