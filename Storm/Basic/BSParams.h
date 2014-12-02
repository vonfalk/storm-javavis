#pragma once
#include "Std.h"
#include "Basic/BSType.h"

namespace storm {
	namespace bs {

		/**
		 * Parameter list.
		 */
		class Params : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Params();

			void STORM_FN add(Auto<TypeName> type);
			void STORM_FN add(Auto<SStr> name);

			vector<Auto<TypeName> > params;
			vector<Auto<SStr> > names;

		protected:
			virtual void output(wostream &to) const;
		};

	}
}
