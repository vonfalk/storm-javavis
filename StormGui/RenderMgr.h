#pragma once

namespace stormgui {

	/**
	 * Singleton class in charge of managing window repaints.
	 */
	class RenderMgr : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Destroy.
		virtual ~RenderMgr();

	private:
		friend RenderMgr *renderMgr(EnginePtr e);

		// Create.
		RenderMgr();

		// The D2D-factory.
		ID2D1Factory *factory;
	};

	// Create/get the singleton render manager.
	RenderMgr *STORM_ENGINE_FN renderMgr(EnginePtr e);

}

