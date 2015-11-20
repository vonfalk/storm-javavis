#include "stdafx.h"
#include "RenderMgr.h"
#include "StormGui.h"
#include "Exception.h"
#include "Painter.h"

namespace stormgui {

#ifdef DEBUG
	static D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_INFORMATION };
#else
	static D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };
#endif
	static UINT deviceFlags = D3D10_CREATE_DEVICE_BGRA_SUPPORT;

	String throwError(const String &msg, HRESULT r) {
		throw GuiError(msg + ::toS(r));
	}


	HRESULT createDevice(ID3D10Device1 **device) {
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

	RenderMgr::RenderMgr() : exiting(false) {
		factory = null;
		device = null;
		giDevice = null;
		giFactory = null;
		writeFactory = null;

		HRESULT h;

		h = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), &options, (void **)&factory);
		if (FAILED(h))
			throwError(L"Failed to create a D2D factory: ", h);

		if (FAILED(h = createDevice(&device)))
			throwError(L"Failed to create a D3D device: ", h);
		if (FAILED(h = device->QueryInterface(__uuidof(IDXGIDevice), (void **)&giDevice)))
			throwError(L"Failed to get the DXGI device: ", h);
		IDXGIAdapter *adapter = null;
		if (FAILED(h = giDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&adapter)))
			throwError(L"Failed to get the DXGIAdapter: ", h);
		if (FAILED(h = adapter->GetParent(__uuidof(IDXGIFactory), (void **)&giFactory)))
			throwError(L"Failed to get the GI factory: ", h);
		::release(adapter);

		DWRITE_FACTORY_TYPE type = DWRITE_FACTORY_TYPE_SHARED;
		if (FAILED(h = DWriteCreateFactory(type, __uuidof(IDWriteFactory), (IUnknown **)&writeFactory)))
			throwError(L"Failed to initialize Direct Write: ", h);
	}

	RenderMgr::~RenderMgr() {
		if (!exiting)
			terminate();

		::release(giDevice);
		::release(device);
		::release(factory);
		::release(giFactory);
		::release(writeFactory);
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

	RenderMgr::RenderInfo RenderMgr::attach(Par<Painter> painter, HWND window) {
		RECT c;
		GetClientRect(window, &c);

		DXGI_SWAP_CHAIN_DESC desc;
		create(desc, window);

		RenderInfo r;
		HRESULT h;

		// TODO: Use CreateSwapChainForHwnd.
		if (FAILED(h = giFactory->CreateSwapChain(device, &desc, &r.swapChain))) {
			r.release();
			throwError(L"Failed to create a swap chain: ", h);
		}

		try {
			r.target = createTarget(r.swapChain);
		} catch (...) {
			r.release();
			throw;
		}

		painters.insert(painter.borrow());
		return r;
	}

	void RenderMgr::detach(Par<Painter> painter) {
		painters.erase(painter.borrow());
	}

	void RenderMgr::resize(RenderInfo &info, Size sz) {
		::release(info.target);

		HRESULT r;
		if (FAILED(r = info.swapChain->ResizeBuffers(1, (UINT)sz.w, (UINT)sz.h, DXGI_FORMAT_UNKNOWN, 0))) {
			throwError(L"Failed to resize buffers: " , r);
		}

		info.target = createTarget(info.swapChain);
	}

	ID2D1RenderTarget *RenderMgr::createTarget(IDXGISwapChain *swapChain) {
		HRESULT h;
		ID2D1RenderTarget *target;
		ComPtr<IDXGISurface> surface;

		if (FAILED(h = swapChain->GetBuffer(0, __uuidof(IDXGISurface), (void **)&surface))) {
			throwError(L"Failed to get the surface: ", h);
		}

		D2D1_RENDER_TARGET_PROPERTIES props = {
			D2D1_RENDER_TARGET_TYPE_DEFAULT,
			{ DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED },
			0, 0, // Dpi
			D2D1_RENDER_TARGET_USAGE_NONE,
			D2D1_FEATURE_LEVEL_10, // Use _DEFAULT?
		};
		if (FAILED(h = factory->CreateDxgiSurfaceRenderTarget(surface.v, &props, &target))) {
			throwError(L"Failed to create a render target: ", h);
		}

		return target;
	}

	void RenderMgr::terminate() {
		exiting = true;
		waitEvent.set();
		exitSema.down();
	}

	void RenderMgr::main() {
		vector<Auto<Painter>> toRedraw;

		while (!exiting) {
			// Will generally not free any backing storage.
			toRedraw.clear();

			// Figure out which we need to redraw this frame. Copy them since others may modify the
			// hash set as soon as we do UThread::leave.
			for (hash_set<Painter *>::iterator i = painters.begin(), end = painters.end(); i != end; ++i) {
				Painter *p = *i;
				if (p->continuous)
					toRedraw.push_back(capture(p));
			}

			for (nat i = 0; i < toRedraw.size(); i++) {
				// TODO: We probably want to wait for VSync once only, and not block this thread while doing so.
				try {
					toRedraw[i]->doRepaint(true);
				} catch (const Exception &e) {
					PLN(L"Error while rendering: " << e);
				} catch (...) {
					PLN(L"Unknown error while rendering.");
				}
				os::UThread::leave();
			}

			if (toRedraw.size() == 0)
				waitEvent.wait();
			waitEvent.clear();
		}

		exitSema.up();
	}

	void RenderMgr::newContinuous() {
		waitEvent.set();
	}

	RenderMgr *renderMgr(EnginePtr e) {
		LibData *d = (stormgui::LibData *)e.v.data();
		if (!d->renderMgr)
			d->renderMgr = CREATE(RenderMgr, e.v);
		return d->renderMgr.ret();
	}

}
