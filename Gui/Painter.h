#pragma once
#include "Handle.h"
#include "Window.h"
#include "Graphics.h"
#include "RenderMgr.h"
#include "Core/WeakSet.h"

namespace gui {
	class RenderResource;

	class Painter : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		STORM_CTOR Painter();

		virtual ~Painter();

		// Called to render the window. Before this call, the output is filled with 'bgColor'.
		// 'size' is the drawing size in device independent units.
		// Return 'true' if you want to be redrawn the next frame as well!
		virtual Bool STORM_FN render(Size size, Graphics *graphics);

		// Background color. Updated on next redraw.
		Color bgColor;

		/**
		 * The following API is intended to be private to the C++ code.
		 */

		// Attach to a specific window.
		void CODECALL attach(Window *to);

		// Detach from the previous window.
		void CODECALL detach();

		// Called when the attached window has been resized.
		void CODECALL resize(Size to);

		// Called when the attached window wants to be repainted.
		void STORM_FN repaint();

		// Get our render target.
		// inline ID2D1RenderTarget *renderTarget() { return target.target; }

		// Add a resource. Resources are invalidated whenever we have to re-create the render target.
		void addResource(RenderResource *resource);
		void removeResource(RenderResource *resource);

	private:
		friend class RenderMgr;

		// Attached HWND.
		Handle attachedTo;

		// Rendering information.
		RenderMgr::RenderInfo target;

		// Graphics object.
		Graphics *graphics;

		// Resources.
		WeakSet<RenderResource> *resources;

		// Registered for continuous repaints in RenderMgr?  If true, then calls to 'repaint' will
		// not do anything, instead we rely on RenderMgr to repaint us every frame.
		Bool continuous;

		// Create any resources connected to the current device.
		void create();

		// Destroy any resources connected to the current device.
		void destroy();

		// Destroy loaded resources.
		void destroyResources();

		// Repaint number (to keep track of when 'redraw' should return and avoid flicker).
		volatile Nat repaintCounter;

		// Do repaints (always).
		void doRepaint(bool waitForVSync);
	};

}
