#pragma once

namespace gui {

	/**
	 * Plaform specific information required to render to the screen.
	 */
	class RenderInfo {
		STORM_VALUE;
	public:

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

		// Release all members.
		inline void release() {
			::release(target());
			::release(swapChain());
		}
		inline bool any() const {
			return swapChain() != null;
		}
#endif
#ifdef GUI_GTK
		inline cairo_surface_t *surface() const {
			return (cairo_surface_t *)first;
		}
		inline void surface(cairo_surface_t *to) {
			first = to;
		}
		inline cairo_t *device() const {
			return (cairo_t *)second;
		}
		inline void device(cairo_t *to) {
			second = to;
		}
		inline void release() {
			if (second)
				cairo_destroy(device());
			if (first)
				cairo_surface_destroy(surface());
		}
		inline bool any() const {
			return device() != null;
		}
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
