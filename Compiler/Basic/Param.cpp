#include "stdafx.h"
#include "Param.h"

namespace storm {
	namespace bs {

		NameParam::NameParam(SrcName *type, Str *name) : type(type), name(name) {}

		wostream &operator <<(wostream &to, NameParam p) {
			StrBuf *b = new (p.name) StrBuf();
			*b << p;
			return to << b->toS();
		}

		StrBuf &operator <<(StrBuf &to, NameParam p) {
			return to << p.type << L" " << p.name;
		}

		ValParam::ValParam(Value type, Str *name) : type(type), name(name) {}

		wostream &operator <<(wostream &to, ValParam p) {
			StrBuf *b = new (p.name) StrBuf();
			*b << p;
			return to << b->toS();
		}

		StrBuf &operator <<(StrBuf &to, ValParam p) {
			return to << p.type << L" " << p.name;
		}

		ValParam resolve(NameParam param, Scope scope) {
			return ValParam(scope.value(param.type), param.name);
		}

		Array<ValParam> *resolve(Array<NameParam> *params, Scope scope) {
			Array<ValParam> *res = new (params) Array<ValParam>();
			res->reserve(params->count());
			for (nat i = 0; i < params->count(); i++)
				res->push(resolve(params->at(i), scope));
			return res;
		}


	}
}
