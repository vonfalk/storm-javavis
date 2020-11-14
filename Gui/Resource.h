#pragma once
#include "Core/TObject.h"
#include "Core/GcArray.h"

namespace gui {

	class Graphics;
	class GraphicsResource;

	/**
	 * A resource used in rendering that might need to create backend-specific resources.
	 *
	 * Any backend specific resources are created lazily, and the Resource keeps track of
	 * potentially multiple such resources for multiple backends.
	 */
	class Resource : public ObjectOn<Render> {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		STORM_CTOR Resource();

		// Destroy the resource for a particular Graphics backend.
		void STORM_FN destroy(Graphics *g);

		// Get a resource for a particular backend.
		GraphicsResource *STORM_FN forGraphics(Graphics *g);

	protected:
		// Called whenever a GraphicsResource needs to be created.
		virtual GraphicsResource *STORM_FN create(Graphics *g) ABSTRACT;

	private:
		// Array typedef for convenience.
		typedef GcArray<GraphicsResource *> Arr;

		// Data. Either an array of elements, or just a GraphicsResource.
		UNKNOWN(PTR_GC) void *data;

		// Description of the data. If LSB is clear, then 'data' is a single element for the
		// identifier in here. Otherwise, it is an array.
		Nat info;

		// Get the element at a particular ID.
		GraphicsResource *get(Nat id);

		// Set the element at a particular ID. Resizes storage if needed.
		void set(Nat id, GraphicsResource *r);

		// Clear the element at a particular ID. Resizes storage if needed.
		void clear(Nat id);
	};


	/**
	 * Some kind of backend-specific resource that is managed by a Resource class. These are not
	 * meant to be used directly, they will be created as needed by subclasses to Resource.
	 */
	class GraphicsResource : public ObjectOn<Render> {
		STORM_CLASS;
	public:
		// Create.
		STORM_CTOR GraphicsResource();

		// Safeguard for destruction.
		~GraphicsResource();

		// Destroy this resource.
		virtual void destroy();
	};

}
