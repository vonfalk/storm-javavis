#pragma once
#include "RenderResource.h"

namespace stormgui {

	/**
	 * Brush (abstract).
	 */
	class Brush : public RenderResource {
		STORM_CLASS;
	public:

		// Get a brush.
		inline ID2D1Brush *brush(Painter *owner, const Size &size) { prepare(size); return get<ID2D1Brush>(owner); }
		inline ID2D1Brush *brush(Painter *owner, const Rect &rect) { prepare(rect); return get<ID2D1Brush>(owner); }

		// Prepare for drawing a bounding box of 'bound'.
		virtual void prepare(const Rect &bound);
		inline void prepare(const Size &size) { prepare(Rect(Point(), size)); }
	};

	/**
	 * Solid brush.
	 */
	class SolidBrush : public Brush {
		STORM_CLASS;
	public:
		STORM_CTOR SolidBrush(Color color);

		virtual void create(Painter *owner, ID2D1Resource **out);

	private:
		Color color;
	};

	/**
	 * A single gradient stop.
	 */
	class GradientStop {
		STORM_VALUE;
	public:
		STORM_CTOR GradientStop(Float position, Color color);

		STORM_VAR Float pos;
		STORM_VAR Color color;
	};

	wostream &operator <<(wostream &to, const GradientStop &s);
	Str *STORM_ENGINE_FN toS(EnginePtr e, GradientStop s);

	/**
	 * Gradient of some kind.
	 */
	class Gradient : public Brush {
		STORM_CLASS;
	public:
		Gradient();
		Gradient(Par<Array<GradientStop>> stops);
		~Gradient();

		// Set the stops.
		void STORM_SETTER stops(Par<Array<GradientStop>> stops);

		// Destroy the stops.
		virtual void destroy();

	protected:
		// Get the stops object.
		ID2D1GradientStopCollection *dxStops(Painter *owner);

	private:
		ID2D1GradientStopCollection *dxObject;
		vector<D2D1_GRADIENT_STOP> myStops;
	};


	/**
	 * Linear gradient.
	 */
	class LinearGradient : public Gradient {
		STORM_CLASS;
	public:
		STORM_CTOR LinearGradient(Par<Array<GradientStop>> stops, Angle angle);

		// Create two stops at 0 and 1.
		STORM_CTOR LinearGradient(Color c1, Color c2, Angle angle);

		// Get the brush.
		inline ID2D1LinearGradientBrush *brush(Painter *owner) { return get<ID2D1LinearGradientBrush>(owner); }

		// Create.
		virtual void create(Painter *owner, ID2D1Resource **out);

		// Prepare.
		virtual void prepare(const Rect &s);

		// The angle.
		STORM_VAR Angle angle;

	private:
		// Last prepared rect.
		Rect prepared;

		// Compute points.
		void compute(const Rect &sz, Point &start, Point &end);
	};

}
