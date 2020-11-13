#include "stdafx.h"
#include "D2D.h"
#include "Exception.h"
#include "Win32Dpi.h"

#ifdef GUI_WIN32

namespace gui {

	static void check(HRESULT r, const wchar *msg) {
		if (FAILED(r)) {
			Engine &e = runtime::someEngine();
			Str *m = TO_S(e, msg << ::toS(r).c_str());
			throw new (e) GuiError(m);
		}
	}

	static void check(Engine &e, HRESULT r, const wchar *msg) {
		if (FAILED(r)) {
			Str *m = TO_S(e, msg << ::toS(r).c_str());
			throw new (e) GuiError(m);
		}
	}

	static HRESULT createDevice(ID3D10Device1 **device) {
		HRESULT r;

		D3D10_DRIVER_TYPE types[] = {
			D3D10_DRIVER_TYPE_HARDWARE,
			D3D10_DRIVER_TYPE_WARP,
		};
		D3D10_FEATURE_LEVEL1 featureLevels[] = {
			D3D10_FEATURE_LEVEL_10_1,
			D3D10_FEATURE_LEVEL_10_0,
			D3D10_FEATURE_LEVEL_9_3,
			D3D10_FEATURE_LEVEL_9_2,
			D3D10_FEATURE_LEVEL_9_1,
		};

		for (nat t = 0; t < ARRAY_COUNT(types); t++) {
			for (nat f = 0; f < ARRAY_COUNT(featureLevels); f++) {
				r = D3D10CreateDevice1(NULL,
									types[t],
									NULL,
									deviceFlags,
									featureLevels[f],
									D3D10_1_SDK_VERSION,
									device);
				if (SUCCEEDED(r))
					return r;
			}
			WARNING(L"Falling back to software rendering, expect bad performance!");
		}

		return r;
	}

	D2DDevice::D2DDevice(Engine &e) : factory(null), device(null), giDevice(null), giFactory(null), writeFactory(null) {
		// Setup DX and DirectWrite.
		HRESULT r;
		r = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, (void **)&factory.v);
		check(e, r, S("Failed to create D2D factory: "));

		r = createDevice(&device.v);
		check(e, r, S("Failed to create a D3D device: "));

		r = device->QueryInterface(__uuidof(IDXGIDevice), (void **)&giDevice.v);
		check(e, r, S("Failed to get the DXGI device: "));

		ComPtr<IDXGIAdapter> adapter = null;
		giDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&adapter.v);
		check(e, r, S("Failed to get the DXGIAdapter: "));

		adapter->GetParent(__uuidof(IDXGIFactory), (void **)&giFactory.v);
		check(e, r, S("Failed to get the DXGI factory: "));

		DWRITE_FACTORY_TYPE type = DWRITE_FACTORY_TYPE_SHARED;
		r = DWriteCreateFactory(type, __uuidof(IDWriteFactory), (IUnknown **)&writeFactory.v);
		check(e, r, S("Failed to initialize Direct Write: "));
	}

	D2DDevice::~D2DDevice() {}

	static void create(DXGI_SWAP_CHAIN_DESC &desc, HWND window) {
		RECT c;
		GetClientRect(window, &c);

		zeroMem(desc);
		desc.BufferCount = 1;
		desc.BufferDesc.Width = c.right - c.left;
		desc.BufferDesc.Height = c.bottom - c.top;
		desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BufferDesc.RefreshRate.Numerator = 60; // TODO: Needed?
		desc.BufferDesc.RefreshRate.Denominator = 1;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.OutputWindow = window;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Windowed = TRUE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // Should improve performance.
	}

	ID2D1RenderTarget *Device::createTarget(IDXGISwapChain *swapChain) {
		ID2D1RenderTarget *target;
		ComPtr<IDXGISurface> surface;

		HRESULT r = swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void **)&surface.v);
		check(r, S("Failed to get the surface: "));

		D2D1_RENDER_TARGET_PROPERTIES props = {
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			{ DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
			0, 0, // DPI -- TODO: We can re-create render targets depending on DPI here to avoid scaling!
			D2D1_RENDER_TARGET_USAGE_NONE,
			D2D1_FEATURE_LEVEL_10, // Use _DEFAULT?
		};
		r = factory->CreateDxgiSurfaceRenderTarget(surface.v, &props, &target);
		check(r, S("Failed to create a render target: "));

		return target;
	}

	D2DSurface *D2DDevice::createSurface(Handle window) {
		DXGI_SWAP_CHAIN_DESC desc;
		create(desc, window.hwnd());

		// TODO: Maybe use CreateSwapChainForHwnd?
		ComPtr<IDXGISwapChain> swapChain;
		HRESULT r = giFactory->CreateSwapChain(device, &desc, &swapChain.v);
		check(r, S("Failed to create a swap chain for a window: "));

		ComPtr<ID2D1RenderTarget> target = createTarget(swapChain.v);

		return new D2DSurface(sz, dpiScale(windowDpi(window.hwnd())), target, swapChain);
	}


	D2DSurface::D2DSurface(Size size, Float scale, D2DDevice *device,
						const ComPtr<ID2D1RenderTarget> &target,
						const ComPtr<IDXGISwapChain> &swap)
		: Surface(size, scale), device(device), target(target), swapChain(swap) {}

	void D2DSurface::resize(Size size) {
		target.clear();

		HRESULT r = swapChain->ResizeBuffers(1, (UINT)size.w, (UINT)size.h, DXGI_FORMAT_UNKNOWN, 0);
		check(r, S("Failed to resize buffer: "));

		target = device->createTarget(swapChain.v);

		this->size = size;
	}

	void D2DSurface::present(bool waitForVSync) {
		HRESULT r = swapChain.Present(waitForVSync ? 1 : 0, 0);
		// TODO: Check for D2DERR_RECREATE_TARGET or DXGI_ERROR_DEVICE_RESET in particular.
		return SUCCEEDED(r);
	}

}

#endif
