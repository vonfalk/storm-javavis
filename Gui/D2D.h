#pragma once
#include "Device.h"
#include "Surface.h"
#include "ComPtr.h"

#ifdef GUI_WIN32

namespace gui {

	/**
	 * Direct 2D device.
	 */
	class D2DDevice : public Device {
	public:
		// Create.
		D2Device(Engine &e);

		// Create a surface.
		virtual D2DSurface *createSurface(Handle window);

		// Create a render target. Called from D2DSurface.
		ID2D1RenderTarget *createTarget(IDXGISwapChain *swapChain);

	private:
		// Factories.
		ComPtr<ID2D1Factory> factory;
		ComPtr<IDXGIFactory> giFactory;
		ComPtr<IDWriteFactory> writeFactory;

		// D3D device and DXGI device.
		ComPtr<ID2D10Device1> device;
		ComPtr<IDXGIDevice> giDevice;
	};

	/**
	 * Direct 2D surface.
	 */
	class D2DSurface : public Surface {
		friend class D2DDevice;
	public:
		// Destroy.
		~D2DSurface();

		// Resize.
		virtual void resize(Size size);

		// Present.
		virtual void present();

	private:
		// Create.
		D2DSurface(Size size, Float scale, D2DDevice *device,
				const ComPtr<ID2D1RenderTarget> &target, const ComPtr<IDXGISwapChain> &swap);

		// DX resources.
		D2DDevice *device;
		ComPtr<ID2D1RenderTarget> renderTarget;
		ComPtr<IDXGISwapChain> swapChain;
	};

}

#endif
