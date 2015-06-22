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
			IDXGISurface *surface;
			IDXGISwapChain *swapChain;

			// Create
			inline RenderInfo() : target(null), surface(null), swapChain(null) {}

			// Release all members.
			inline void release() {
				::release(target);
				::release(surface);
				::release(swapChain);
			}
		};

		// Attach a Painter.
		RenderInfo attach(Par<Painter> painter, HWND window);

		// Detach a Painter.
		void detach(Par<Painter> painter);

	private:
		friend RenderMgr *renderMgr(EnginePtr e);

		// Create.
		RenderMgr();

		// The D2D-factory.
		ID2D1Factory *factory;
		IDXGIFactory *giFactory;

		// D3D device and dxgi device.
		ID3D10Device1 *device;
		IDXGIDevice *giDevice;

		// Live painters (weak pointers).
		hash_set<Painter *> painters;

	};

	// Create/get the singleton render manager.
	RenderMgr *STORM_ENGINE_FN renderMgr(EnginePtr e);

}

