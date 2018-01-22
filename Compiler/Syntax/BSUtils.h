#pragma once

#include "Compiler/Basic/Named.h"


namespace storm {
	namespace syntax {

		/**
		 * Convenience functions for calling members using the Basic Storm backend.
		 */

		bs::Expr *callMember(Scope scope, Str *name, bs::Expr *me);
		bs::Expr *callMember(Scope scope, Str *name, bs::Expr *me, bs::Expr *param);
		bs::Expr *callMember(const SrcPos &pos, Scope scope, Str *name, bs::Expr *me);
		bs::Expr *callMember(const SrcPos &pos, Scope scope, Str *name, bs::Expr *me, bs::Expr *param);
		bs::Expr *callMember(Scope scope, const wchar *name, bs::Expr *me);
		bs::Expr *callMember(Scope scope, const wchar *name, bs::Expr *me, bs::Expr *param);
		bs::Expr *callMember(const SrcPos &pos, Scope scope, const wchar *name, bs::Expr *me);
		bs::Expr *callMember(const SrcPos &pos, Scope scope, const wchar *name, bs::Expr *me, bs::Expr *param);


	}
}
