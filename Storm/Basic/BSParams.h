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

			vector<Auto<TypeName> > params;

		protected:
			virtual void output(wostream &to) const;
		};

	}
}
