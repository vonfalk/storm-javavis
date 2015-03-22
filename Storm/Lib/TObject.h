#pragma once
#include "Object.h"

namespace storm {

	STORM_PKG(TObject);

	/**
	 * Root object for threaded objects. This object acts like 'Object'
	 * when the object is associated with a statically named thread (so there is
	 * no memory overhead for this). In C++, this object inherits from Object
	 * so that we do not have to duplicate a lot of code, however this is hidden
	 * in Storm and the as<> operator. Otherwise, it would be possible to call
	 * any member function of 'Object' in the wrong threading context by simply
	 * casting to 'Object' first.
	 */
	class TObject : public STORM_HIDDEN(Object) {
		STORM_CLASS;
	public:
		STORM_CTOR TObject();
		STORM_CTOR TObject(TObject *copy);

		// This is more or less a dummy object to make the Object and TObject to be separate.
	};

}
