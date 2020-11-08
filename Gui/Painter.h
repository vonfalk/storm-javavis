#pragma once
#include "Handle.h"
#include "Window.h"
#include "WindowGraphics.h"
#include "RenderMgr.h"
#include "Core/WeakSet.h"
#include "Core/Lock.h"

namespace gui {
	class App;
	class RenderResource;

	/**
	 * Paramters to 'redraw' calls. Not exposed to Storm.
	 */
	struct RepaintParams {
#ifdef GUI_GTK
		GdkWindow *target;
		GtkWidget *widget;
		cairo_t *ctx;
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

		// Called from Storm to repaint the window.
		void STORM_FN repaint();

		// Background color. Updated on next redraw.
		Color bgColor;

		// Add a resource. Resources are invalidated whenever we have to re-create the render target.
		void addResource(RenderResource *resource);
		void removeResource(RenderResource *resource);

#ifdef GUI_WIN32
		// Get our render target.
		inline ID2D1RenderTarget *renderTarget() { return target.target(); }
#endif
#ifdef GUI_GTK
		inline CairoSurface *surface() { return target.surface(); }
#endif

		/**
		 * The following functions (the ones starting with 'uiXxx') are intended to be called
		 * directly from Window on the Ui thread (not the Render thread). This is for two
		 * reasons. Firstly, convenience inside the Window class. But more importantly, it is since
		 * some backends require to do some work on the Ui thread to synchronize drawing.
		 */

		// Attach to a window.
		void uiAttach(Window *to);

		// Detach from the currently attached window.
		void uiDetach();

		// Called when the attached window has been resized. "scale" is used to scale the initial
		// layer of the canvas in case we're running in a non-default DPI mode.
		void uiResize(Size size, Float scale);

		// Called when the attached window wants to be repainted. The parameter passed contains
		// OS-specific data.
		void uiRepaint(RepaintParams *params);

	private:
		friend class RenderMgr;

		// Attached HWND.
		Handle attachedTo;

		// Rendering information.
		RenderInfo target;

		// Graphics object.
		WindowGraphics *graphics;

		// App object.
		App *app;

		// Render manager.
		RenderMgr *mgr;

		// Lock for the target surface.
		Lock *lock;

		// Resources.
		WeakSet<RenderResource> *resources;

		// Registered for continuous repaints in RenderMgr?  If true, then calls to 'repaint' will
		// not do anything, instead we rely on RenderMgr to repaint us every frame.
		Bool continuous;

		// Are we attached to anything? If we get detached while waiting to render the screen, we
		// need to be able to detect that.
		inline bool attached() const { return graphics != null; }

		// Is the window ready to be rendered to?
		bool windowReady(Handle window);

		// Ready to render? Called by the render manager to determine if the painter is ready to
		// render when it is in continuous mode. Some implementations may need to wait for some
		// event even when they are in continuous mode. If a painter ever returns false from
		// ready(), it needs to call 'RenderMgr::painterReady' when it becomes ready again.
		bool ready();

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

		// Attach to a specific window.
		void CODECALL attach(Window *to);

		// Detach from the previous window.
		void CODECALL detach();

		// Called when the attached window has been resized.
		void CODECALL resize(Size to, Float scale);

		// Called when the attached window wants to be repainted. The parameter passed is
		// OS-specific data.
		void CODECALL repaintI(RepaintParams *params);

		// Called before and after the actual work inside 'repaint' is done. Used for platform specific calls.
		void beforeRepaint(RepaintParams *handle);
		void afterRepaint();

		// Called from the UI thread.
		void uiAfterRepaint(RepaintParams *handle);

		// Wait for a new frame to be rendered (used during continuous updates).
		void waitForFrame();

		// Do repaints (always).
		void doRepaint(bool waitForVSync, bool fromDraw);

		// Do the platform specific of the repaint cycle.
		bool doRepaintI(bool waitForVSync, bool fromDraw);
	};

}
