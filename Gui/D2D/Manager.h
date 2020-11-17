#pragma once
#include "Gui/GraphicsMgr.h"
#include "D2D.h"

namespace gui {

	class D2DManager : public GraphicsMgrRaw {
		STORM_CLASS;
	public:
#ifdef GUI_WIN32
		// Create.
		D2DManager(D2DSurface &surface);
#endif

		// Create resources:
		virtual void create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup);

		// Update resources:
		virtual void update(SolidBrush *brush, void *resource);
		virtual void update(LinearGradient *brush, void *resource);
		virtual void update(RadialGradient *brush, void *resource);

	private:
		// Our surface.
		D2DSurface &surface;

#ifdef GUI_WIN32
		// Check HRESULT:s.
		void check(HRESULT r, const wchar *msg);
#endif
	};

}
