#pragma once
#include "Handle.h"
#include "Window.h"
#include "Graphics.h"
#include "RenderMgr.h"
#include "Core/WeakSet.h"

namespace gui {
	class RenderResource;

	/**
	 * Paramters to 'redraw' calls. Not exposed to Storm.
	 */
	struct RepaintParams {
#ifdef GUI_GTK
		GdkWindow *target;
		GtkWidget *widget;
#endif
	};

	/**
	 * Object responsible for painting window contents. Create a subclass and override
	 * 'render'. Then attach it to a Window.
	 */
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

		// Called when the attached window wants to be repainted. The parameter passed is
		// OS-specific data.
		void CODECALL repaint(RepaintParams *params);

		// Add a resource. Resources are invalidated whenever we have to re-create the render target.
		void addResource(RenderResource *resource);
		void removeResource(RenderResource *resource);

#ifdef GUI_WIN32
		// Get our render target.
		inline ID2D1RenderTarget *renderTarget() { return target.target(); }
#endif
	private:
		friend class RenderMgr;

		// Attached HWND.
		Handle attachedTo;

		// Rendering information.
		RenderInfo target;

		// Graphics object.
		Graphics *graphics;

		// Resources.
		WeakSet<RenderResource> *resources;

		// Registered for continuous repaints in RenderMgr?  If true, then calls to 'repaint' will
		// not do anything, instead we rely on RenderMgr to repaint us every frame.
		Bool continuous;

		// Are we currently rendering?
		Bool rendering;

		// Create any resources connected to the current device.
		void create();

		// Destroy any resources connected to the current device.
		void destroy();

		// Destroy loaded resources.
		void destroyResources();

		// Repaint number (to keep track of when 'redraw' should return and avoid flicker).
		volatile Nat repaintCounter;

		// Last repaint shown on the screen. Used to determine if we are ready to draw the next frame.
		volatile Nat currentRepaint;

		// Called before and after the actual work inside 'repaint' is done. Used for platform specific calls.
		void beforeRepaint(RepaintParams *handle);
		void afterRepaint(RepaintParams *handle);

		// Wait for a new frame to be rendered (used during continuous updates).
		void waitForFrame();

		// Do repaints (always).
		void doRepaint(bool waitForVSync);

		// Do the platform specific of the repaint cycle.
		bool doRepaintI(bool waitForVSync);
	};

}
