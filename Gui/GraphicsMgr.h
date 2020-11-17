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

	// Helper for the macro below.
#define DEFINE_MGR_FN(CLASS, TYPE)										\
	void CLASS::create(TYPE *, void *&, Resource::Cleanup &) {			\
		throw new (this) NotSupported(S("create(") S(#TYPE) S(")"));	\
	}																	\
	void CLASS::update(TYPE *, void *) {								\
		throw new (this) NotSupported(S("update(") S(#TYPE) S(")"));	\
	}

	// Define functions for a subclass to GraphicsMgrRaw. Useful since implementations of the
	// GraphicsMgrRaw can not generally be compiled on the "wrong" platform.
#define DEFINE_GRAPHICS_MGR_FNS(CLASS)			\
	DEFINE_MGR_FN(CLASS, SolidBrush)			\
	DEFINE_MGR_FN(CLASS, LinearGradient)		\
	DEFINE_MGR_FN(CLASS, RadialGradient)

}
