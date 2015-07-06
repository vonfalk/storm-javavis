#pragma once
#include "Graphics.h"
#include "RenderMgr.h"

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
		STORM_CTOR Painter();
		~Painter();

		// Called to render the window. Before this call, the output is filled with 'bgColor'.
		// 'size' is the drawing size in device independent units.
		// Return 'true' if you want to be redrawn the next frame as well!
		virtual Bool STORM_FN render(Size size, Par<Graphics> graphics);

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
		void STORM_FN repaint();

		// Get our render target.
		inline ID2D1RenderTarget *renderTarget() { return target.target; }

		// Add a resource. Resources are invalidated whenever we have to re-create the render target.
		void addResource(Par<RenderResource> resource);
		void removeResource(Par<RenderResource> resource);

	private:
		friend class RenderMgr;

		// Attached HWND.
		HWND attachedTo;

		// Rendering information.
		RenderMgr::RenderInfo target;

		// Graphics object.
		Auto<Graphics> graphics;

		// Resources.
		hash_set<RenderResource *> resources;

		// Registered for continuous repaints in RenderMgr?
		// If true, then calls to 'repaint' will not do anything, but we rely on RenderMgr to repaint us every frame.
		bool continuous;

		// Create any resources connected to the current device.
		void create();

		// Destroy any resources connected to the current device.
		void destroy();

		// Destroy loaded resources.
		void destroyResources();

		// Repaint number (to keep track of when 'redraw' should return and avoid flicker).
		volatile nat repaintCounter;

		// Do repaints (always).
		void doRepaint(bool waitForVSync);
	};

}

