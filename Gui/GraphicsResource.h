#pragma once
#include "Resource.h"
#include "Brush.h"

namespace gui {

	/**
	 * A resource manager for a Graphics object.
	 *
	 * An instance of this object defines how a Graphics object handles its temporary resources.
	 *
	 * This class defines the underlying interface for C++. It is not usable from Storm. Use
	 * GraphicsRes for that purpose.
	 */
	class GraphicsMgrRaw : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create resources:
		virtual void create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup);

		// Update resources:
		virtual void update(SolidBrush *brush, void *resource);
		virtual void update(LinearGradient *brush, void *resource);
		virtual void update(RadialGradient *brush, void *resource);
	};

	/**
	 * Resource usable from Storm.
	 */
	class GraphicsResource : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR GraphicsResource() {}

		// Update the resource.
		virtual void STORM_FN update() {}
	};

	/**
	 * Wrapper usable from Storm.
	 */
	class GraphicsMgr : public GraphicsMgrRaw {
		STORM_ABSTRACT_CLASS;
	public:
		// Storm interface:
		virtual GraphicsResource *STORM_FN create(SolidBrush *brush) ABSTRACT;
		virtual GraphicsResource *STORM_FN create(LinearGradient *brush) ABSTRACT;
		virtual GraphicsResource *STORM_FN create(RadialGradient *brush) ABSTRACT;

		// Create resources:
		virtual void create(SolidBrush *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(LinearGradient *brush, void *&result, Resource::Cleanup &cleanup);
		virtual void create(RadialGradient *brush, void *&result, Resource::Cleanup &cleanup);

		// Update resources:
		virtual void update(SolidBrush *brush, void *resource);
		virtual void update(LinearGradient *brush, void *resource);
		virtual void update(RadialGradient *brush, void *resource);
	};

}
