#pragma once
#include "Core/Set.h"
#include "Core/WeakSet.h"
#include "Core/TObject.h"
#include "Core/EnginePtr.h"
#include "Core/Event.h"
#include "Core/Sema.h"
#include "Handle.h"
#include "GraphicsId.h"
#include "Device.h"

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

		// Allocate a Graphics ID.
		Nat allocId();

		// Free an ID.
		void freeId(Nat id);

		// Attach a Painter, creates a surface.
		Surface *attach(Painter *painter, Handle window);

		// Detach a Painter.
		void detach(Painter *painter);

		// Notify that a new painter is ready to repaint. OK to call from other threads.
		void painterReady();

		// Get the text manager.
		TextMgr *text() const { return textMgr; }

	private:
		friend RenderMgr *renderMgr(EnginePtr e);

		// Create.
		RenderMgr();

		// The underlying device.
		UNKNOWN(PTR_NOGC) Device *device;

		// The underlying text manager.
		UNKNOWN(PTR_NOGC) TextMgr *textMgr;

		// Identifiers for Graphics objects.
		UNKNOWN(PTR_NOGC) IdMgr *idMgr;

		// Live painters. TODO? Weak set?
		Set<Painter *> *painters;

		// Event to wait for either: new continuous windows, or: termination.
		Event *waitEvent;

		// Semaphore to synchronize shutdown.
		Sema *exitSema;

		// Exiting?
		Bool exiting;

		// Main entry point for the thread.
		void main();

		// Friend.
		friend os::Thread spawnRenderThread(Engine &e);
	};

	// Create/get the singleton render manager.
	RenderMgr *STORM_FN renderMgr(EnginePtr e);

	os::Thread spawnRenderThread(Engine &e);

}
