#include "stdafx.h"
#include "BSUtils.h"
#include "Exception.h"
#include "Type.h"

namespace storm {
	namespace syntax {

		using namespace bs;

		Expr *callMember(Scope scope, Str *name, Expr *me) {
			return callMember(scope, name, me, null);
		}

		Expr *callMember(Scope scope, Str *name, Expr *me, Expr *param) {
			Value type = me->result().type();
			if (type == Value())
				throw new (name) InternalError(S("Can not call members of 'void'."));

			Actuals *actual = new (me) Actuals();
			Array<Value> *params = new (me) Array<Value>();
			params->push(thisPtr(type.type));
			actual->add(me);
			if (param) {
				params->push(param->result().type());
				actual->add(param);
			}

			SimplePart *part = new (me) SimplePart(name, params);
			Named *found = type.type->find(part, scope);
			Function *toCall = as<Function>(found);
			if (!toCall)
				throw new (name) InternalError(TO_S(name, part << S(" was not found!")));

			return new (me) FnCall(me->pos, scope, toCall, actual);
		}

		Expr *callMember(const SrcPos &pos, Scope scope, Str *name, Expr *me) {
			return callMember(pos, scope, name, me, null);
		}

		Expr *callMember(const SrcPos &pos, Scope scope, Str *name, Expr *me, Expr *param) {
			try {
				return callMember(scope, name, me, param);
			} catch (const InternalError *e) {
				throw new (e) SyntaxError(pos, e->message());
			}
		}

		Expr *callMember(Scope scope, const wchar *name, Expr *me) {
			return callMember(scope, name, me, null);
		}

		Expr *callMember(Scope scope, const wchar *name, Expr *me, Expr *param) {
			return callMember(scope, new (me) Str(name), me, param);
		}

		Expr *callMember(const SrcPos &pos, Scope scope, const wchar *name, Expr *me) {
			return callMember(pos, scope, name, me, null);
		}

		Expr *callMember(const SrcPos &pos, Scope scope, const wchar *name, Expr *me, Expr *param) {
			return callMember(pos, scope, new (me) Str(name), me, param);
		}


	}
}
