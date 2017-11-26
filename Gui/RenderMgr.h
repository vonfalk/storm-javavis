#pragma once
#include "Core/Set.h"
#include "Core/WeakSet.h"
#include "Core/TObject.h"
#include "Core/EnginePtr.h"
#include "Core/Event.h"
#include "Core/Sema.h"
#include "Handle.h"
#include "RenderInfo.h"

namespace gui {
	class Painter;
	class Resource;

	/**
	 * Singleton class in charge of managing window repaints.
	 */
	class RenderMgr : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Shutdown the rendering thread.
		void terminate();

		// Attach a resource to this rendermgr.
		void attach(Resource *resource);

		// Attach a Painter.
		RenderInfo attach(Painter *painter, Handle window);

		// Detach a Painter.
		void detach(Painter *painter);

		// Resize the RenderInfo to a new size. 'target' will be re-created.
		void resize(RenderInfo &info, Size size);

		// Main thread entry point.
		void main();

		// Notify that a new window wants continuous repaints.
		void newContinuous();

#ifdef GUI_WIN32
		// Get the DWrite factory object.
		inline IDWriteFactory *dWrite() { return writeFactory; }

		// Get the D2D factory object.
		inline ID2D1Factory *d2d() { return factory; }
#endif
	private:
		friend RenderMgr *renderMgr(EnginePtr e);

		// Create.
		RenderMgr();

		// The D2D-factory.
		ID2D1Factory *factory;
		IDXGIFactory *giFactory;
		IDWriteFactory *writeFactory;

		// D3D device and dxgi device.
		ID3D10Device1 *device;
		IDXGIDevice *giDevice;

		// For OpenGL: The global context and the global display.
		UNKNOWN(PTR_NOGC) EGLDisplay glDisplay;
		UNKNOWN(PTR_NOGC) EGLContext glContext;
		UNKNOWN(PTR_NOGC) cairo_device_t *glDevice;

		// Live painters. TODO? Weak set?
		Set<Painter *> *painters;

		// Live resources that needs cleaning before termination.
		WeakSet<Resource> *resources;

		// Event to wait for either: new continuous windows, or: termination.
		Event *waitEvent;

		// Semaphore to synchronize shutdown.
		Sema *exitSema;

		// Exiting?
		Bool exiting;

		// Platform specific initialization.
		void init();

		// Platform specific destruction.
		void destroy();

#ifdef GUI_WIN32
		// Create a render target.
		ID2D1RenderTarget *createTarget(IDXGISwapChain *swapChain);
#endif
#ifdef GUI_GTK
		// Create the global context if neccessary.
		cairo_device_t *createDevice(GtkWidget *widget);
#endif
	};

	// Create/get the singleton render manager.
	RenderMgr *STORM_FN renderMgr(EnginePtr e);

	os::Thread spawnRenderThread(Engine &e);

}
