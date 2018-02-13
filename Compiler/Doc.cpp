#include "stdafx.h"
#include "Doc.h"
#include "Named.h"
#include "Exception.h"
#include "Core/Join.h"

namespace storm {

	/**
	 * DocParams.
	 */

	DocParam::DocParam(Str *name, Value type) : name(name), type(type) {}

	StrBuf &operator <<(StrBuf &to, DocParam p) {
		if (p.name->any())
			return to << p.type << S(" ") << p.name;
		else
			return to << p.type;
	}

	wostream &operator <<(wostream &to, DocParam p) {
		if (p.name->any())
			return to << p.type << L" " << p.name;
		else
			return to << p.type;
	}


	/**
	 * Doc.
	 */

	Doc::Doc(Str *name, Array<DocParam> *params, Str *body) :
		name(name), params(params), body(body) {}

	void Doc::deepCopy(CloneEnv *env) {
		cloned(name, env);
		cloned(params, env);
		cloned(body, env);
		// 'entity' is a TObject.
	}

	void Doc::toS(StrBuf *to) const {
		*to << name << S("(") << join(params, S(", ")) << S("):\n");
		*to << body;
	}


	/**
	 * NamedDoc.
	 */

	NamedDoc::NamedDoc() {}

	Doc *NamedDoc::get() {
		throw InternalError(L"NamedDoc::get not overridden.");
	}

}
