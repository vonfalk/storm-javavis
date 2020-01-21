#include "stdafx.h"
#include "GlobalVar.h"
#include "Scope.h"
#include "Function.h"
#include "Named.h"
#include "Resolve.h"
#include "Compiler/Variable.h"
#include "Compiler/NamedThread.h"
#include "Compiler/Exception.h"
#include "Compiler/Lib/Fn.h"

namespace storm {
	namespace bs {

		GlobalVarDecl::GlobalVarDecl(Scope scope, SrcName *type, syntax::SStr *name, SrcName *thread) :
			scope(scope), type(type), name(name), thread(thread), initExpr(null) {}

		void GlobalVarDecl::init(syntax::Node *initTo) {
			this->initExpr = initTo;
		}

		void GlobalVarDecl::toS(StrBuf *to) const {
			*to << type << S(" ") << name->v << S(" on ") << thread;
		}

		Named *GlobalVarDecl::doCreate() {
			Scope fScope = fileScope(scope, name->pos);

			Value type = fScope.value(this->type);
			NamedThread *thread = as<NamedThread>(fScope.find(this->thread));
			if (!thread) {
				Str *msg = TO_S(engine(), S("The name ") << this->thread << S(" does not refer to a named thread."));
				throw new (this) SyntaxError(this->thread->pos, msg);
			}

			Function *init = createInitializer(type, scope, thread);
			return new (this) GlobalVar(name->v, type, thread, pointer(init));
		}

		void GlobalVarDecl::doResolve(Named *entity) {
			if (GlobalVar *v = as<GlobalVar>(entity)) {
				v->create();
			}
		}

		Function *GlobalVarDecl::createInitializer(Value type, Scope scope, NamedThread *thread) {
			GlobalInitializer *r = new (this) GlobalInitializer(this->type->pos, type, scope, initExpr);
			// Trick the function into believing that it has a proper name (just like
			// lambdas). Necessary in order to be able to call 'runOn', which is done when we create
			// function pointers to the function.
			r->parentLookup = scope.top;
			r->runOn(thread);
			return r;
		}


		GlobalInitializer::GlobalInitializer(SrcPos pos, Value type, Scope scope, MAYBE(syntax::Node *) initExpr) :
			BSRawFn(type, new (engine()) syntax::SStr(S("init")), new (engine()) Array<ValParam>(), null),
			pos(pos), scope(scope), initExpr(initExpr) {}

		FnBody *GlobalInitializer::createBody() {
			FnBody *body = new (this) FnBody(this, scope);

			if (initExpr) {
				// Use the expression.
				body->add(syntax::transformNode<Expr, Block *>(initExpr, body));
			} else {
				// Create an instance using the default constructor.
				body->add(defaultCtor(pos, scope, result.type));
			}

			return body;
		}

	}
}
