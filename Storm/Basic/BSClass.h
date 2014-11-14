#pragma once
#include "Std.h"

namespace storm {
	namespace bs {

		class Class : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Class(Auto<Str> name, Auto<Str> content);
		};


		class Tmp : public Object {
			STORM_CLASS;
		public:
			STORM_CTOR Tmp();

			void STORM_FN add(Auto<Class> c);
		};
	}
}
