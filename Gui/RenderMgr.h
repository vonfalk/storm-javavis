#pragma once
#include "Core/Set.h"
#include "Core/WeakSet.h"
#include "Core/TObject.h"
#include "Core/EnginePtr.h"
#include "Core/Event.h"
#include "Core/Sema.h"
#include "Handle.h"
#include "RenderInfo.h"
#include "Device.h"
#include "DxDevice.h"
#include "CairoDevice.h"

namespace gui {
	class Painter;
	class Resource;

	/**
	 * Singleton class in charge of managing continuous window repaints, and associated resources.
	 */
	class RenderMgr : public ObjectOn<Ui> {
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
		void resize(RenderInfo &info, Size size, Float scale);

		// Notify that a new painter is ready to repaint.
		void painterReady();

		// Get the device type.
		inline DeviceType deviceType() { return device->type(); }

#ifdef GUI_WIN32
		// Get the DWrite factory object.
		inline IDWriteFactory *dWrite() { return device->dWrite(); }

		// Get the D2D factory object.
		inline ID2D1Factory *d2d() { return device->d2d(); }
#endif
#ifdef GUI_GTK
		// Get a pango context.
		inline PangoContext *pango() { return device->pango(); }

		// Create the context the first time it is rendered, if needed.
		RenderInfo create(RepaintParams *params);
#endif
	private:
		friend RenderMgr *renderMgr(EnginePtr e);

		// Create.
		RenderMgr();

		// The underlying device.
		Device *device;

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

		// Main thread entry point for the monitoring thread.
		void CODECALL main();

#ifdef GUI_GTK
		// Create the global context if neccessary.
		cairo_device_t *createDevice(GtkWidget *widget);
#endif
	};

	// Create/get the singleton render manager.
	RenderMgr *STORM_FN renderMgr(EnginePtr e);

}
