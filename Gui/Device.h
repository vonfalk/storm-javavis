#pragma once
#include "Handle.h"
#include "Env.h"

namespace gui {

	class Surface;
	class TextMgr;
	class SurfaceWorkaround;

	/**
	 * A generic interface to some rendering device.
	 *
	 * An instance of this class represents some policy of how to create a rendering context,
	 * possibly with some set of associated resources.
	 *
	 * This interface is made to support at least:
	 * - (GDI+)
	 * - Direct 2D (Windows)
	 * - Cairo (Both HW and SW)
	 * - Skia (HW, Linux)
	 *
	 * All functions here are called from the Render thread. Furthermore, "createSurface" is called
	 * while holding a lock from the UI thread, so that it can access the windows freely.
	 */
	class Device : NoCopy {
	public:
		// Create a suitable device for this system.
		static Device *create(Engine &e);

		// Create.
		Device() {}

		// Create a surface to draw on associated with a window.
		// Might return "null", which means that the window is not yet ready for being rendered to.
		virtual Surface *createSurface(Handle window) = 0;

		// Create a text manager compatible with this device.
		virtual TextMgr *createTextMgr() = 0;

#ifdef GUI_GTK
		// Get the draw widget for Gtk. Utility function as it is needed in all Gtk devices.
		static GtkWidget *drawWidget(Engine &e, Handle handle);
#endif
	};


#ifdef GUI_GTK
	/**
	 * A slight specialization of the Device above for OpenGL contexts in Gtk.
	 *
	 * In Gtk, we can share GL contexts for all widgets that share the same GdkWindow. We strive to
	 * do that since that allows us to share resources between them, so that it is more likely that
	 * we hit the fast-path in the render resources (i.e. only one backend resource for each
	 * object). For different GdkWindows, Gdk will create separate GL contexts for painting, so we
	 * cannot share resources between those (even though it would be technically possible).
	 *
	 * This class expects derived classes to implement "createSurface(GLDevice::Context *)",
	 * and that surfaces unref the context when they are done.
	 *
	 * When a context is created, the driver also attempts to apply any workarounds required for the
	 * hardware accelerated parts that are deemed necessary.
	 */
	class GLDevice : public Device {
	public:
		// Create.
		GLDevice(Engine &e);

		// Destroy.
		~GLDevice();

		/**
		 * Representation of GL context.
		 *
		 * Reference counted.
		 */
		class Context : private ::NoCopy {
			friend class GLDevice;
		public:
			// Create. Used by GLDevice.
			Context(GLDevice *owner, GdkWindow *window, GdkGLContext *context);

			// Destroy.
			~Context();

			// Add a reference.
			void ref() {
				atomicIncrement(refs);
			}

			// Remove a reference.
			void unref() {
				if (atomicDecrement(refs) == 0)
					delete this;
			}

			// The window.
			GdkWindow *window;

			// The GL context.
			GdkGLContext *context;

			// Allocated ID for this context. As we don't know how the actual device works, we don't
			// allocate an ID here, we just provide storage for it (initialized to zero).
			Nat id;

		private:
			// Reference counting.
			size_t refs;

			// Owning GLDevice. Set to zero when the device is destroyed.
			GLDevice *owner;

			// Workarounds to apply for this context.
			SurfaceWorkaround *workarounds;
		};

		// Engine (we need it, so why not make it public).
		Engine &e;

		// Create a surface. Implements the sharing logic.
		virtual Surface *createSurface(Handle window);

	protected:
		// Override to create a Context.
		virtual Context *createContext(GdkWindow *window, GdkGLContext *context);

		// Create an appropriate surface. Expects that the implementation will eventually unreference Context.
		virtual Surface *createSurface(GtkWidget *drawWidget, Context *context) = 0;

	private:
		// Mapping between GdkWindow their Contexts, so we know when to create a new context. We
		// don't have refs to these contexts, so they may go out of scope. The Context will notify
		// us of that situation.
		typedef hash_map<GdkWindow *, Context *> Map;
		Map context;

		// Create a GL context.
		GdkGLContext *createContext(GdkWindow *window);
	};

#endif
}
