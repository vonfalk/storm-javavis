#pragma once
#include "Object.h"
#include "Shared/Geometry/Point.h"
#include "Shared/Geometry/Size.h"
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

		// Specific size (truncated).
		STORM_CTOR Image(geometry::Size size);
		STORM_CTOR Image(Nat w, Nat h);

		// Destroy.
		~Image();

		// Get size.
		inline Nat STORM_FN width() { return w; }
		inline Nat STORM_FN height() { return h; }
		geometry::Size STORM_FN size();

		// TODO: Replace at leas one of these with foo[pt] = color;
		// Get pixel at point.
		Color STORM_FN get(Nat x, Nat y);
		Color STORM_FN get(geometry::Point at);

		// Set pixel.
		void STORM_FN set(Nat x, Nat y, Color c);
		void STORM_FN set(geometry::Point p, Color c);

		// Raw buffer information:

		// Stride, difference between each row (in bytes).
		nat stride() const;

		// Size of buffer.
		nat bufferSize() const;

		// Raw buffer access.
		byte *buffer();
		byte *buffer(nat x, nat y);

	private:
		// Data.
		byte *data;

		// Size.
		nat w, h;

		// Compute offset of pixel.
		nat offset(nat x, nat y);
	};

}
