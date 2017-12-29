#pragma once

namespace gui {

#ifdef GUI_GTK
	class GlSurface;
#endif

	/**
	 * Plaform specific information required to render to the screen.
	 */
	class RenderInfo {
		STORM_VALUE;
	public:
		// Size of the surface we're rendering to.
		// TODO: Remove?
		Size size;

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
		inline void swapChain(IDGXISwapChain *to) {
			second = to;
		}
		inline bool any() const {
			return swapChain() != null;
		}

		// Release all members.
		void release()
#endif
#ifdef GUI_GTK
		inline cairo_t *target() const {
			return (cairo_t *)first;
		}
		inline void target(cairo_t *cairo) {
			first = cairo;
		}

		inline GlSurface *surface() const {
			return (GlSurface *)second;
		}
		inline void surface(GlSurface *s) {
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
		}

	private:
		UNKNOWN(PTR_NOGC) void *first;
		UNKNOWN(PTR_NOGC) void *second;
	};

}
