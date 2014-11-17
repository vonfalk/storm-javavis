#pragma once
#include "Std.h"
#include "SyntaxObject.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class Class : public SObject {
			STORM_CLASS;
		public:
			// Create class 'name' with contents 'content'.
			STORM_CTOR Class(Auto<SStr> name, Auto<SStr> content);

		private:
			
		};


		class Tmp : public SObject {
			STORM_CLASS;
		public:
			STORM_CTOR Tmp();

			void STORM_FN add(Auto<Class> c);
		};
	}
}
