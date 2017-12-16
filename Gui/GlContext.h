#pragma once
#include "nanovg.h"

#ifdef GUI_GTK

namespace gui {

	/**
	 * Base class for an OpenGL context. The GL context behaves a bit different depending on which
	 * windowing system is used. EGL works on both Wayland and X11, but not through X11 forwarding,
	 * so we use GLX as a fallback.
	 *
	 * TODO: Context sharing between all GlContext objects available.
	 *
	 * Note: We assume that the GlContext objects are used only from the rendering thread.
	 *
	 * Note: These are allocated using new and delete rather than on the GC.
	 */
	class GlContext : NoCopy {
	public:
		virtual ~GlContext();

		// The nanovg context.
		NVGcontext *nvg;

		// Activate this context.
		void activate();

		// Swap buffers of the context.
		virtual void swapBuffers() = 0;

		// Create a GlContext for the given window. Note: the newly created context will be
		// activated during creation.
		static GlContext *create(GdkWindow *window);

	protected:
		// Create.
		GlContext();

		// Call before the GL context is destroyed so that the nanovg context can be destroyed
		// properly.
		void destroy();

		// Set this context as the active one.
		virtual void setActive() = 0;

	private:
		// Which context is active at the moment? May be null.
		static GlContext *active;
	};


	/**
	 * EGL implementation.
	 */
	class EglContext : public GlContext {
	public:
		virtual ~EglContext();

		// Swap buffers.
		virtual void swapBuffers();

		// Create an EGL context. Returns null on failure.
		static EglContext *create(GdkWindow *window);

	protected:
		// Set as the active context.
		virtual void setActive();

	private:
		EglContext(EGLDisplay display);

		// Display data. One for each display. We use a single EGLContext for all surfaces within a
		// single display.
		class DisplayData {
		public:
			DisplayData(EGLDisplay display);
			~DisplayData();

			// The display.
			EGLDisplay display;

			// The context. Null until the display has been initialized.
			EGLContext context;

			// The configuration used for this context.
			EGLConfig config;

			// Initialize the data if needed. Returns true on success.
			bool initialize();

			// Increase/decrease reference count.
			inline void addRef() {
				atomicIncrement(refs);
			}

			inline void release() {
				if (atomicDecrement(refs) == 0)
					delete this;
			}

		private:
			// Number of references.
			size_t refs;
		};

		// The data for our display.
		DisplayData *display;

		// Window we're drawing to.
		EGLNativeWindowType window;

		// Surface we're drawing to.
		EGLSurface surface;

		// All display objects in the system.
		typedef std::map<EGLDisplay, DisplayData *> DisplayMap;
		static DisplayMap displays;
	};


	/**
	 * GLX implementation.
	 */
	class GlxContext : public GlContext {
	public:
		virtual ~GlxContext();

		// Swap buffers.
		virtual void swapBuffers();

		// Create a GLX context. Returns null on failure.
		static GlxContext *create(GdkWindow *window);

	protected:
		// Set as the active context.
		virtual void setActive();

	private:
		GlxContext(Display *display, ::Window window);

		// Display data. One for each display. We use a single EGLContext for all surfaces within a
		// single display.
		class DisplayData {
		public:
			DisplayData(Display *display);
			~DisplayData();

			// The display.
			Display *display;

			// The context. Null until the display has been initialized.
			GLXContext context;

			// Initialize the data if needed. Returns true on success.
			bool initialize();

			// Increase/decrease reference count.
			inline void addRef() {
				atomicIncrement(refs);
			}

			inline void release() {
				if (atomicDecrement(refs) == 0)
					delete this;
			}

		private:
			// Number of references.
			size_t refs;
		};

		// The data for our display.
		DisplayData *display;

		// Window we're drawing to.
		::Window window;

		// All display objects in the system.
		typedef std::map<Display *, DisplayData *> DisplayMap;
		static DisplayMap displays;
	};

	/**
	 * Render to a texture.
	 */
	class TextureContext : public GlContext {
	public:
		// Create inside another context.
		TextureContext(GlContext *inside, Size size);

		// Destroy.
		virtual ~TextureContext();

		// Swap buffers. Does nothing.
		virtual void swapBuffers();

		// Resize.
		void resize(Size size);

		// Get a NVG image id for the texture.
		int nvgImage();

	protected:
		// Set as active.
		virtual void setActive();

	private:
		// Owning context.
		GlContext *owner;

		// The of this texture.
		Size mySize;

		// Frame buffer object.
		GLuint framebuffer;

		// Texture we're rendering to.
		GLuint texture;

		// Depth- and stencil buffer.
		GLuint depth;

		// NVG image id. -1 if not yet created.
		int nvgId;

		// Create/resize the buffers.
		void createBuffers(Size size);
	};

}

#else

namespace gui {

	// Dummy type definition for use in class declarations.
	class GlContext;

}

#endif
