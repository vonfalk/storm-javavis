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

		// Attach a Painter.
		ID2D1HwndRenderTarget *attach(Par<Painter> painter, HWND window);

		// Detach a Painter.
		void detach(Par<Painter> painter);

	private:
		friend RenderMgr *renderMgr(EnginePtr e);

		// Create.
		RenderMgr();

		// The D2D-factory.
		ID2D1Factory *factory;

		// Live painters (weak pointers).
		hash_set<Painter *> painters;
	};

	// Create/get the singleton render manager.
	RenderMgr *STORM_ENGINE_FN renderMgr(EnginePtr e);

}

