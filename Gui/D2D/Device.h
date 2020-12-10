#pragma once
#include "Gui/Device.h"
#include "Gui/Surface.h"
#include "ComPtr.h"

#ifdef GUI_WIN32

namespace gui {

	/**
	 * Direct 2D device.
	 */
	class D2DDevice : public Device {
		friend class D2DSurface;
	public:
		// Create.
		D2DDevice(Engine &e);

		// Create a surface.
		virtual Surface *createSurface(Handle window);

		// Create a text manager.
		virtual TextMgr *createTextMgr();

		// We're hardware accelerated.
		virtual bool isHardware() const { return true; }

	private:
		// Factories.
		ComPtr<ID2D1Factory> factory;
		ComPtr<IDXGIFactory> giFactory;

		// D3D device and DXGI device.
		ComPtr<ID3D10Device1> device;
		ComPtr<IDXGIDevice> giDevice;

		// Create a render target. Called from D2DSurface.
		ID2D1RenderTarget *createTarget(IDXGISwapChain *swapChain);

		// Engine.
		Engine &e;

		// The ID allocated for the device.
		Nat id;
	};

	/**
	 * Direct 2D surface.
	 */
	class D2DSurface : public Surface {
		friend class D2DDevice;
	public:
		// Destroy.
		~D2DSurface();

		// Create a suitable Graphics object.
		virtual WindowGraphics *createGraphics(Engine &e);

		// Resize.
		virtual void resize(Size size, Float scale);

		// Present.
		virtual PresentStatus present(bool waitForVSync);

		// Get the render target.
		ID2D1RenderTarget *target() const { return renderTarget.v; }

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
