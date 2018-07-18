#include "stdafx.h"
#include "GlobalVar.h"
#include "Scope.h"
#include "Compiler/Variable.h"
#include "Compiler/NamedThread.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		GlobalVarDecl::GlobalVarDecl(Scope scope, SrcName *type, syntax::SStr *name, SrcName *thread) :
			scope(scope), type(type), name(name), thread(thread) {}

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

			return new (this) GlobalVar(name->v, type, thread);
		}

	}
}
