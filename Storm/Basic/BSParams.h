#pragma once
#include "Std.h"
#include "Basic/BSType.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		/**
		 * Parameter list.
		 */
		class Params : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Params();

			void STORM_FN add(Par<TypeName> type);
			void STORM_FN add(Par<SStr> name);

			// Add a 'this' as the first parameter.
			void addThis(Type *t);

			vector<Auto<TypeName> > params;
			vector<Auto<SStr> > names;

			vector<String> cNames() const;
			vector<Value> cTypes(const Scope &scope) const;

		protected:
			virtual void output(wostream &to) const;

			// 'this' pointer?
			Type *thisType;
		};

	}
}
