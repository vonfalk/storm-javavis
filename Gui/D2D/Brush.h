#pragma once
#include "Gui/Resource.h"
#include "Gui/GraphicsResource.h"
#include "Gui/Brush.h"
#include "D2D.h"

namespace gui {

	STORM_PKG(impl);

#ifndef GUI_WIN32
	// To make this file compile on Linux:
	class ID2D1Brush;
#endif

	class D2DBrush : public GraphicsResource {
		STORM_CLASS;
	public:
		D2DBrush();
		~D2DBrush();

		// The DX brush.
		ID2D1Brush *brush;

		// Destroy the brush.
		virtual void STORM_FN destroy();
	};

	class D2DSolidBrush : public D2DBrush {
		STORM_CLASS;
	public:
		D2DSolidBrush(D2DSurface &surface, SolidBrush *src);

		virtual void STORM_FN update();
	private:
		SolidBrush *src;
	};

	class D2DLinearGradient : public D2DBrush {
		STORM_CLASS;
	public:
		D2DLinearGradient(D2DSurface &surface, LinearGradient *src);

		virtual void STORM_FN update();
	private:
		LinearGradient *src;
		Nat version;
	};

	class D2DRadialGradient : public D2DBrush {
		STORM_CLASS;
	public:
		D2DRadialGradient(D2DSurface &surface, RadialGradient *src);

		virtual void STORM_FN update();
	private:
		RadialGradient *src;
		Nat version;
	};

}
