#include "stdafx.h"
#include "DxDevice.h"
#include "Exception.h"

#ifdef GUI_WIN32
namespace gui {

#ifdef DEBUG
	static D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_INFORMATION };
#else
	static D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };
#endif
	static UINT deviceFlags = D3D10_CREATE_DEVICE_BGRA_SUPPORT;

	void throwError(Engine &e, const wchar *msg, HRESULT r) {
		Str *m = TO_S(e, msg << ::toS(r).c_str());
		throw new (e) GuiError(m);
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
	}

	Device::Device(Engine &e) {
		factory = null;
		device = null;
		giDevice = null;
		giFactory = null;
		writeFactory = null;

		// Setup DirectX, Direct2D and DirectWrite.
		HRESULT h;
		h = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, (void **)&factory);
		if (FAILED(h))
			throwError(e, S("Failed to create a D2D factory: "), h);

		if (FAILED(h = createDevice(&device)))
			throwError(e, S("Failed to create a D3D device: "), h);
		if (FAILED(h = device->QueryInterface(__uuidof(IDXGIDevice), (void **)&giDevice)))
			throwError(e, S("Failed to get the DXGI device: "), h);
		IDXGIAdapter *adapter = null;
		if (FAILED(h = giDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&adapter)))
			throwError(e, S("Failed to get the DXGIAdapter: "), h);
		if (FAILED(h = adapter->GetParent(__uuidof(IDXGIFactory), (void **)&giFactory)))
			throwError(e, S("Failed to get the GI factory: "), h);
		::release(adapter);

		DWRITE_FACTORY_TYPE type = DWRITE_FACTORY_TYPE_SHARED;
		if (FAILED(h = DWriteCreateFactory(type, __uuidof(IDWriteFactory), (IUnknown **)&writeFactory)))
			throwError(e, S("Failed to initialize Direct Write: "), h);
	}

	Device::~Device() {
		::release(giDevice);
		::release(device);
		::release(factory);
		::release(giFactory);
		::release(writeFactory);
	}

	RenderInfo Device::attach(Handle window) {
		RECT c;
		GetClientRect(window.hwnd(), &c);

		DXGI_SWAP_CHAIN_DESC desc;
		create(desc, window.hwnd());

		RenderInfo r;
		HRESULT h;

		// TODO: Use CreateSwapChainForHwnd.
		IDXGISwapChain *swapChain = null;
		if (FAILED(h = giFactory->CreateSwapChain(device, &desc, &swapChain))) {
			r.release();
			throwError(runtime::someEngine(), S("Failed to create a swap chain: "), h);
		}

		r.swapChain(swapChain);

		try {
			r.target(createTarget(swapChain));
		} catch (...) {
			r.release();
			throw;
		}

		r.size = Size(Float(desc.BufferDesc.Width), Float(desc.BufferDesc.Height));
		return r;
	}

	void Device::resize(RenderInfo &info, Size sz) {
		if (info.target())
			info.target()->Release();
		info.target(null);

		HRESULT r;
		if (FAILED(r = info.swapChain()->ResizeBuffers(1, (UINT)sz.w, (UINT)sz.h, DXGI_FORMAT_UNKNOWN, 0))) {
			throwError(runtime::someEngine(), S("Failed to resize buffers: ") , r);
		}

		info.size = sz;
		info.target(createTarget(info.swapChain()));
	}

	ID2D1RenderTarget *Device::createTarget(IDXGISwapChain *swapChain) {
		HRESULT h;
		ID2D1RenderTarget *target;
		ComPtr<IDXGISurface> surface;

		if (FAILED(h = swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void **)&surface))) {
			throwError(runtime::someEngine(), S("Failed to get the surface: "), h);
		}

		D2D1_RENDER_TARGET_PROPERTIES props = {
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			{ DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
			0, 0, // Dpi
			D2D1_RENDER_TARGET_USAGE_NONE,
			D2D1_FEATURE_LEVEL_10, // Use _DEFAULT?
		};
		if (FAILED(h = factory->CreateDxgiSurfaceRenderTarget(surface.v, &props, &target))) {
			throwError(runtime::someEngine(), S("Failed to create a render target: "), h);
		}

		return target;
	}

}
#endif
