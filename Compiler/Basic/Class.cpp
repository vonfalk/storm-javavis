#include "stdafx.h"
#include "Class.h"
#include "Compiler/Exception.h"
#include "Compiler/TypeCtor.h"
#include "Compiler/TypeDtor.h"
#include "Compiler/Engine.h"
#include "Function.h"
#include "Ctor.h"
#include "Access.h"
#include "Doc.h"

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

		Class::AddState::AddState() : ctor(false), copyCtor(false), deepCopy(false), assign(false) {}

		bool Class::loadAll() {
			if (!allowLazyLoad)
				return false;

			ClassBody *body = syntax::transformNode<ClassBody, Class *>(this->body, this);
			body->prepare();

			AddState added;

			// Add the named objects first.
			for (Nat i = 0; i < body->items->count(); i++) {
				addMember(body->items->at(i), added);
			}

			// Poke the classes to tell them they can load their super types now.
			for (Nat i = 0; i < body->items->count(); i++) {
				if (Class *c = as<Class>(body->items->at(i)))
					c->lookupTypes();
			}

			// Add the wrapped items.
			for (Nat i = 0; i < body->wraps->count(); i++) {
				addMember(body->wraps->at(i)->transform(this), added);
			}


			// Add default members as required.
			if (needsDestructor(this))
				add(new (engine) TypeDefaultDtor(this));

			if (!added.ctor)
				add(classDefaultCtor(this));

			if (!added.copyCtor && runOn().state == RunOn::any)
				add(new (engine) TypeCopyCtor(this));

			if (!added.deepCopy && runOn().state == RunOn::any)
				add(new (engine) TypeDeepCopy(this));

			if ((typeFlags & typeValue) && !added.assign)
				add(new (engine) TypeAssign(this));

			// TODO: Add a default toS for values somehow.

			for (Nat i = 0; i < body->templates->count(); i++) {
				add(body->templates->at(i));
			}

			// We do not need the syntax tree anymore!
			this->body = null;

			// Super needs to be called!
			return Type::loadAll();
		}

		void Class::addMember(Named *item, AddState &state) {
			if (*item->name == Type::CTOR) {
				if (item->params->count() == 2 && item->params->at(0) == item->params->at(1))
					state.copyCtor = true;
				state.ctor = true;
			} else if (*item->name == S("deepCopy")) {
				if (item->params->count() == 2 && item->params->at(1).type == CloneEnv::stormType(this))
					state.deepCopy = true;
			} else if (*item->name == S("=")) {
				if (item->params->count() == 2 && item->params->at(1).type == this)
					state.assign = true;
			}

			add(item);
		}


		/**
		 * Wrap.
		 */

		MemberWrap::MemberWrap(syntax::Node *node) : node(node), visibility(null) {}

		Named *MemberWrap::transform(Class *owner) {
			Named *n = syntax::transformNode<Named, Class *>(node, owner);
			if (visibility)
				apply(node->pos, n, visibility);
			if (docPos.any())
				applyDoc(docPos, n);
			return n;
		}


		/**
		 * Body.
		 */

		ClassBody::ClassBody() {
			items = new (this) Array<Named *>();
			wraps = new (this) Array<MemberWrap *>();
			templates = new (this) Array<Template *>();

			// TODO: Default to 'private' instead?
			defaultVisibility = engine().visibility(Engine::vPublic);
		}

		void ClassBody::add(Named *i) {
			if (!i->visibility)
				i->visibility = defaultVisibility;
			items->push(i);
		}

		void ClassBody::add(MemberWrap *w) {
			if (!w->visibility)
				w->visibility = defaultVisibility;
			wraps->push(w);
		}

		void ClassBody::add(Template *t) {
			templates->push(t);
		}

		void ClassBody::add(Visibility *v) {
			defaultVisibility = v;
		}

		void ClassBody::add(TObject *o) {
			if (Named *n = as<Named>(o))
				add(n);
			else if (MemberWrap *w = as<MemberWrap>(o))
				add(w);
			else if (Template *t = as<Template>(o))
				add(t);
			else if (Visibility *v = as<Visibility>(o))
				add(v);
			else
				throw InternalError(L"Not a suitable type to ClassBody.add(): " + ::toS(runtime::typeOf(o)->identifier()));
		}

		void ClassBody::prepare() {}

		/**
		 * Members.
		 */

		MemberVar *classVar(Class *owner, SrcName *type, syntax::SStr *name) {
			return new (owner) MemberVar(name->v, owner->scope.value(type), owner);
		}


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

		BSFunction *STORM_FN classAssign(Class *owner,
										SrcPos pos,
										syntax::SStr *name,
										Array<NameParam> *params,
										syntax::Node *body) {

			BSFunction *f = new (owner) BSFunction(Value(),
												name,
												resolve(params, owner, owner->scope),
												owner->scope,
												null, // thread
												body);
			f->make(fnAssign);
			return f;
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
