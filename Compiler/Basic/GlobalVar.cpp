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
			if (!thread)
				throw SyntaxError(this->thread->pos, L"The name " + ::toS(this->thread) +
								L" does not refer to a named thread.");

			Function *init = createInitializer(type, scope);
			return new (this) GlobalVar(name->v, type, thread, pointer(init));
		}

		Function *GlobalVarDecl::createInitializer(Value type, Scope scope) {
			BSTreeFn *fn = new (this) BSTreeFn(type, new (this) syntax::SStr(S("init")),
											new (this) Array<ValParam>(), null);
			FnBody *body = new (this) FnBody(fn, scope);
			fn->body(body);

			if (initExpr) {
				// Use the initialization if we have one.
				body->add(syntax::transformNode<Expr, Block *>(initExpr, body));
			} else {
				// Create an instance using the default constructor.
				body->add(defaultCtor(this->type->pos, scope, type.type));
			}

			// Trick the function into believing that it has a proper name (just like
			// lambdas). Necessary in order to be able to call 'runOn', which is done when we create
			// function pointers to the function.
			fn->parentLookup = scope.top;
			return fn;
		}

	}
}
