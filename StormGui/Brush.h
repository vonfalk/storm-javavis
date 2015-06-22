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
		inline ID2D1Brush *brush(Painter *owner) { return get<ID2D1Brush>(owner); }
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
	 * TODO? Make so that it respects the geometry of the drawn figure?
	 */
	class LinearGradient : public Gradient {
		STORM_CLASS;
	public:
		STORM_CTOR LinearGradient(Par<Array<GradientStop>> stops, Point start, Point end);

		// Get the brush.
		inline ID2D1LinearGradientBrush *brush(Painter *owner) { return get<ID2D1LinearGradientBrush>(owner); }

		// Create.
		virtual void create(Painter *owner, ID2D1Resource **out);

		// Get/set start and end points.
		Point STORM_FN start();
		Point STORM_FN end();
		void STORM_SETTER start(Point p);
		void STORM_SETTER end(Point p);

	private:
		Point myStart, myEnd;
	};

}
