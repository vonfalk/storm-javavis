#pragma once
#include "Core/StrBuf.h"
#include "Core/Str.h"
#include "Compiler/Name.h"
#include "Compiler/Scope.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * One formal parameter to a function.
		 */
		class NameParam {
			STORM_VALUE;
		public:
			STORM_CTOR NameParam(SrcName *type, Str *name);

			SrcName *type;
			Str *name;
		};

		wostream &operator <<(wostream &to, NameParam p);
		StrBuf &STORM_FN operator<<(StrBuf &to, NameParam p);


		class ValParam {
			STORM_VALUE;
		public:
			STORM_CTOR ValParam(Value type, Str *name);

			Value type;
			Str *name;
		};

		wostream &operator <<(wostream &to, ValParam p);
		StrBuf &STORM_FN operator<<(StrBuf &to, ValParam p);

		// Get parameters as Values.
		ValParam STORM_FN resolve(NameParam param, Scope scope);
		Array<ValParam> *STORM_FN resolve(Array<NameParam> *params, Scope scope);
		Array<ValParam> *STORM_FN resolve(Array<NameParam> *params, Type *me, Scope scope);
		Array<Value> *STORM_FN values(Array<ValParam> *params);

	}
}
