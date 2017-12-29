#pragma once
#include "RenderInfo.h"
#include "Handle.h"

#ifdef GUI_WIN32
namespace gui {

	/**
	 * Implements the logic of creating and accessing a device in DirectX on Windows.
	 */
	class Device : NoCopy {
	public:
		// Create.
		Device(Engine &e);

		// Destroy.
		~Device();

		// Attach a painter.
		RenderInfo attach(Handle window);

		// Resize the target of a painter.
		void resize(RenderInfo &info, Size size);

		// Get the DWrite factory object.
		inline IDWriteFactory *dWrite() { return writeFactory; }

		// Get the D2D factory object.
		inline ID2D1Factory *d2d() { return factory; }

	private:
		// The D2D-factory.
		ID2D1Factory *factory;
		IDXGIFactory *giFactory;
		IDWriteFactory *writeFactory;

		// D3D device and dxgi device.
		ID3D10Device1 *device;
		IDXGIDevice *giDevice;

		// Create a render target.
		ID2D1RenderTarget *createTarget(IDXGISwapChain *swapChain);
	};

}
#endif
