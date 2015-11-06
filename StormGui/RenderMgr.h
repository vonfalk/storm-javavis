#pragma once

namespace stormgui {
	class Painter;

	/**
	 * Singleton class in charge of managing window repaints.
	 */
	class RenderMgr : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Destroy.
		virtual ~RenderMgr();

		// Values returned from 'attach'
		struct RenderInfo {
			ID2D1RenderTarget *target;
			IDXGISwapChain *swapChain;

			// Create
			inline RenderInfo() : target(null), swapChain(null) {}

			// Release all members.
			inline void release() {
				::release(target);
				::release(swapChain);
			}
		};

		// Shutdown the rendering thread.
		void terminate();

		// Attach a Painter.
		RenderInfo attach(Par<Painter> painter, HWND window);

		// Detach a Painter.
		void detach(Par<Painter> painter);

		// Resize the RenderInfo to a new size. 'target' will be re-created.
		void resize(RenderInfo &info, Size size);

		// Main thread entry point.
		void main();

		// Notify that a new window wants continuous repaints.
		void newContinuous();

		// Get the DWrite factory object.
		inline IDWriteFactory *dWrite() { return writeFactory; }

		// Get the D2D factory object.
		inline ID2D1Factory *d2d() { return factory; }

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

		// Live painters (weak pointers).
		hash_set<Painter *> painters;

		// Create a render target.
		ID2D1RenderTarget *createTarget(IDXGISwapChain *swapChain);

		// Event to wait for either: new continuous windows, or: termination.
		os::Event waitEvent;

		// Semaphore to synchronize shutdown.
		os::Sema exitSema;

		// Exiting?
		bool exiting;
	};

	// Create/get the singleton render manager.
	RenderMgr *STORM_ENGINE_FN renderMgr(EnginePtr e);

}

