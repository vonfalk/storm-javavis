#pragma once
#include "Core/Object.h"
#include "Core/Geometry/Point.h"
#include "Core/Geometry/Size.h"
#include "Color.h"

namespace storm {
	STORM_PKG(graphics);

	/**
	 * A 32-bit RGBA image.
	 */
	class Image : public Object {
		STORM_CLASS;
	public:
		// Empty image.
		STORM_CTOR Image();

		// Copy.
		Image(Image *o);

		// Specific size (truncated).
		STORM_CTOR Image(geometry::Size size);
		STORM_CTOR Image(Nat w, Nat h);

		// Get size.
		inline Nat STORM_FN width() { return w; }
		inline Nat STORM_FN height() { return h; }
		geometry::Size STORM_FN size();

		// TODO: Replace at least one of these with foo[pt] = color;
		// Get pixel at point.
		Color STORM_FN get(Nat x, Nat y);
		Color STORM_FN get(geometry::Point at);

		// Set pixel.
		void STORM_FN set(Nat x, Nat y, Color c);
		void STORM_FN set(geometry::Point p, Color c);

		// Raw buffer information:

		// Stride, difference between each row (in bytes).
		Nat stride() const;

		// Size of buffer.
		Nat bufferSize() const;

		// Raw buffer access.
		byte *buffer();
		byte *buffer(Nat x, Nat y);

	private:
		// Data.
		GcArray<Byte> *data;

		// Size.
		Nat w;
		Nat h;

		// Compute offset of pixel.
		Nat offset(Nat x, Nat y);
	};

}
