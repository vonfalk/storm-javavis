#include "stdafx.h"
#include "GraphicsMgr.h"
#include "Core/Exception.h"

namespace gui {

	DEFINE_GRAPHICS_MGR_FNS(GraphicsMgrRaw);

#define DEFINE_MGR_FN(CLASS, TYPE)										\
	void CLASS::create(TYPE *t, void *&result, Resource::Cleanup &cleanup) { \
		cleanup = null;													\
		result = create(t);												\
	}																	\
	void CLASS::update(TYPE *, void *resource) {						\
		((GraphicsResource *)resource)->update();						\
	}

	FOR_GRAPHICS_MGR_FNS(DEFINE_MGR_FN, GraphicsMgr)

}
