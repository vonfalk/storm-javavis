#pragma once

namespace gui {

#ifdef GUI_GTK
	class CairoSurface;
#endif

	class WindowGraphics;

	/**
	 * Plaform specific information required to render to the screen.
	 */
	class RenderInfo {
		STORM_VALUE;
	public:
		// Size of the surface we're rendering to.
		Size size;

		// Scale of the surface (e.g. if we want to emulate a lower resolution when dealing with
		// nonstandard DPI settings).
		Float scale;

		// Create a Graphics object for this render info.
		WindowGraphics *createGraphics(Engine &e) const;

#ifdef GUI_WIN32
		inline ID2D1RenderTarget *target() const {
			return (ID2D1RenderTarget *)first;
		}
		inline void target(ID2D1RenderTarget *to) {
			first = to;
		}

		inline IDXGISwapChain *swapChain() const {
			return (IDXGISwapChain *)second;
		}
		inline void swapChain(IDXGISwapChain *to) {
			second = to;
		}
		inline bool any() const {
			return swapChain() != null;
		}

		// Release all members.
		void release();
#endif
#ifdef GUI_GTK
		inline cairo_t *target() const {
			return (cairo_t *)first;
		}
		inline void target(cairo_t *cairo) {
			first = cairo;
		}

		inline CairoSurface *surface() const {
			return (CairoSurface *)second;
		}
		inline void surface(CairoSurface *s) {
			second = s;
		}
		inline bool any() {
			return target() != null;
		}

		void release();

#endif

		// Create
		inline RenderInfo() {
			first = null;
			second = null;
			scale = 1;
		}

	private:
		UNKNOWN(PTR_NOGC) void *first;
		UNKNOWN(PTR_NOGC) void *second;
	};

}
