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
	 *
	 * Note: The painter always has some kind of internal double-buffering going on.
	 */
	class Painter : public ObjectOn<Ui> {
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
		 * The following functions are called from the Window attached to this painter.
		 */

		// Called when we're attached to a new window.
		void onAttach(Window *to);

		// Called when we're detached from a window.
		void onDetach();

		// Called when the attached window has been resized. "scale" is used to scale the initial
		// layer of the canvas in case we're running in a non-default DPI mode.
		void onResize(Size size, Float scale);

		// Called when the attached window wants to be repainted. The parameter contains OS-specific
		// data.
		void onRepaint(RepaintParams *params);

		// Check the device type.
		DeviceType getDeviceType() const { return deviceType; }

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

		// Lock for the target surface. Used to make sure that we don't re-enter the render function
		// in case of thread switches.
		Lock *lock;

		// Resources.
		WeakSet<RenderResource> *resources;

		// Device type, so we don't have to ask all the time.
		DeviceType deviceType;

		// Registered for continuous repaints in RenderMgr?  If true, then calls to 'repaint' will
		// not do anything, instead we rely on RenderMgr to repaint us every frame.
		Bool continuous;

		// Check if the Window is ready to be attached to. In Gtk+, widgets don't have a backing
		// window until they are realized.
		bool isReady(Handle window);

		// Are we attached to anything? If we get detached while waiting to render the screen, we
		// need to be able to detect that.
		inline bool attached() const { return graphics != null; }

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

		// Image version in the buffer.
		volatile Nat repaintBuffer;

		// Image version shown on the screen.
		volatile Nat repaintScreen;

		// Wait for a new frame to be rendered (used during continuous updates).
		void waitForFrame();

		// Paint a new image into the buffer. Handles registering repaints etc.
		void doRepaint();

		// Do the platform specific of the repaint cycle.
		bool doRepaintI();

		// Present the current frame (assuming raw device mode).
		void present(bool waitForVSync);

		// Present the current frame by blitting it to the window (assuming buffered mode).
		void blit(RepaintParams *params);

		// Helper to invalidate the associated window through 'attachedTo'.
		void invalidateWindow();
	};

}
