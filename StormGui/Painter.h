#pragma once
#include "Graphics.h"

namespace stormgui {
	class Window;
	class RenderResource;

	/**
	 * Painter interface for window graphics. Running in a separate thread for now.
	 * Attach it to a window, and it will receive rendering notifications.
	 */
	class Painter : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// When continuous is set, you will receive rendering notifications once every frame.
		STORM_CTOR Painter(Bool continuous);
		~Painter();

		// Called to render the window. Before this call, the output is filled with 'bgColor'.
		// 'size' is the drawing size in device independent units.
		virtual void STORM_FN render(Size size, Par<Graphics> graphics);

		// Background color. Updated on next redraw.
		STORM_VAR Color bgColor;

		/**
		 * The following API is intended to be private to the C++ code.
		 */

		// Attach to a specific window.
		void CODECALL attach(Par<Window> to);

		// Detach from the previous window.
		void CODECALL detach();

		// Called when the attached window has been resized.
		void CODECALL resize(Size to);

		// Called when the attached window wants to be repainted.
		void CODECALL repaint();

		// Get our render target.
		inline ID2D1RenderTarget *renderTarget() { return target; }

		// Add a resource. Resources are invalidated whenever we have to re-create the render target.
		void addResource(Par<RenderResource> resource);
		void removeResource(Par<RenderResource> resource);

	private:
		// Attached HWND.
		HWND attachedTo;

		// Current render target.
		ID2D1HwndRenderTarget *target;

		// Graphics object.
		Auto<Graphics> graphics;

		// Resources.
		hash_set<RenderResource *> resources;

		// Create any resources connected to the current device.
		void create();

		// Destroy any resources connected to the current device.
		void destroy();
	};

}

