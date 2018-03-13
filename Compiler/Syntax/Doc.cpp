#include "stdafx.h"
#include "Doc.h"
#include "Production.h"

namespace storm {
	namespace syntax {

		SyntaxDoc::SyntaxDoc(SrcPos docPos, Named *entity) : bs::BSDoc(docPos, entity) {}

		Doc *SyntaxDoc::get() {
			Doc *r = bs::BSDoc::get();

			if (ProductionType *p = as<ProductionType>(entity)) {
				StrBuf *newDoc = new (this) StrBuf();
				*newDoc << p->production << S("\n\n");
				*newDoc << r->body;
				r->body = newDoc->toS();
			}

			return r;
		}

	}
}
