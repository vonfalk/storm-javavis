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
						throw SyntaxError(declared, L"Can not find the thread " + ::toS(thread) + L".");

					setThread(t);
					thread = null;
				}

			} catch (...) {
				allowLazyLoad = true;
				throw;
			}
			allowLazyLoad = true;
		}

		void Class::decorate(SrcName *decorator) {
			if (!decorators)
				decorators = new (this) Array<SrcName *>();

			decorators->push(decorator);
		}

		Class::AddState::AddState() : ctor(false), copyCtor(false), deepCopy(false), assign(false) {}

		bool Class::loadAll() {
			if (!allowLazyLoad)
				return false;

			ClassBody *body = syntax::transformNode<ClassBody, Class *>(this->body, this);
			body->prepareItems();

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

			body->prepareWraps();

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

			// Call any decorators.
			if (decorators) {
				for (Nat i = 0; i < decorators->count(); i++) {
					SrcName *name = decorators->at(i);
					Name *params = name->parent();
					params->add(new (this) SimplePart(name->last()->name, Value(Class::stormType(engine))));

					Function *found = as<Function>(scope.find(params));
					if (!found)
						throw SyntaxError(name->pos, L"Could not find a decorator named " + ::toS(name) + L" in the current scope.");

					if (found->result != Value())
						throw SyntaxError(name->pos, L"Decorators may not return a value.");

					// Call the function...
					typedef void (*Fn)(Class *);
					Fn fn = (Fn)found->pointer();
					(*fn)(this);
				}
			}

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

		ClassBody::ClassBody(Class *owner) : owner(owner) {
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

		void ClassBody::prepareItems() {}

		void ClassBody::prepareWraps() {}

		/**
		 * Members.
		 */

		MemberVar *classVar(Class *owner, SrcName *type, syntax::SStr *name) {
			return new (owner) MemberVar(name->v, owner->scope.value(type), owner);
		}


		BSFunction *classFn(Class *owner,
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

		Function *classAbstractFn(Class *owner,
								SrcPos pos,
								syntax::SStr *name,
								Name *result,
								Array<NameParam> *params,
								syntax::Node *options) {

			BSRawFn *f = new (owner) BSAbstractFn(owner->scope.value(result, pos),
												name,
												resolve(params, owner, owner->scope));

			syntax::transformNode<void, Class *, BSRawFn *>(options, owner, f);

			if (f->fnFlags() & fnAbstract)
				return f;

			throw SyntaxError(pos, L"A function without implementation must be marked using ': abstract'.");
		}

		BSFunction *classAssign(Class *owner,
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

		BSCtor *classCtor(Class *owner,
						SrcPos pos,
						Array<NameParam> *params,
						syntax::Node *body) {

			return new (owner) BSCtor(resolve(params, owner, owner->scope),
									owner->scope,
									body,
									pos);
		}

		BSCtor *classCastCtor(Class *owner,
							SrcPos pos,
							Array<NameParam> *params,
							syntax::Node *body) {

			BSCtor *r = classCtor(owner, pos, params, body);
			r->makeAutoCast();
			return r;
		}


		BSCtor *classDefaultCtor(Class *owner) {
			Array<NameParam> *params = CREATE(Array<NameParam>, owner);

			return classCtor(owner, owner->declared, params, null);
		}

	}
}
