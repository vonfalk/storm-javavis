#include "stdafx.h"
#include "Decl.h"
#include "Doc.h"
#include "Compiler/Function.h"
#include "Compiler/Exception.h"

namespace storm {
	namespace bs {

		NamedDecl::NamedDecl() : visibility(null) {}

		Named *NamedDecl::create() {
			Named *r = doCreate();
			r->visibility = visibility;

			// Apply documentation to functions, even if no comment is given. If not, functions
			// without parameters will not show parameter names in the documentation.
			if (docPos.any() || as<Function>(r))
				applyDoc(docPos, r);

			return r;
		}

		Named *NamedDecl::doCreate() {
			throw InternalError(L"Please override 'doCreate'!");
		}

	}
}
