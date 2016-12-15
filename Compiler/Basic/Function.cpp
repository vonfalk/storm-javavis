#include "stdafx.h"
#include "Function.h"
#include "Exception.h"

namespace storm {
	namespace bs {

		FunctionDecl::FunctionDecl(Scope scope,
								SrcName *result,
								syntax::SStr *name,
								Array<NameParam> *params,
								syntax::Node *body)
			: scope(scope),
			  name(name),
			  result(result),
			  params(params),
			  thread(null),
			  body(body) {}

		FunctionDecl::FunctionDecl(Scope scope,
								SrcName *result,
								syntax::SStr *name,
								Array<NameParam> *params,
								SrcName *thread,
								syntax::Node *body)
			: scope(scope),
			  name(name),
			  result(result),
			  params(params),
			  thread(thread),
			  body(body) {}

		Function *FunctionDecl::createFn() {
			Value result = scope.value(this->result);
			NamedThread *thread = null;

			if (this->thread) {
				if (NamedThread *t = as<NamedThread>(scope.find(this->thread))) {
					thread = t;
				} else {
					throw SyntaxError(this->thread->pos, ::toS(this->thread) + L" is not a thread.");
				}
			}

			assert(false);
			// return new (this) BSFunction(result, name, params, scope, thread, body);
			return null;
		}

	}
}
