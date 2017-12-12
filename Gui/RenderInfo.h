#pragma once
#include "GlContext.h"

namespace gui {

	/**
	 * Plaform specific information required to render to the screen.
	 */
	class RenderInfo {
		STORM_VALUE;
	public:
		// Size of the surface we're rendering to.
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
		inline GlContext *context() const {
			return (GlContext *)first;
		}
		inline void context(GlContext *context) {
			first = context;
		}
		inline void release() {
			delete context();
			context(null);
		}
		inline bool any() const {
			return context() != null;
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
