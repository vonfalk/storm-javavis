#pragma once
#include "Std.h"
#include "SyntaxObject.h"
#include "Type.h"

namespace storm {
	namespace bs {
		STORM_PKG(lang.bs);

		class Class : public Type {
			STORM_CLASS;
		public:
			// Create class 'name' with contents 'content'.
			STORM_CTOR Class(SrcPos pos, Auto<SStr> name, Auto<SStr> content);

		};

	}
}
