#pragma once
#include "Core/TObject.h"
#include "Core/GcArray.h"

namespace gui {

	class Graphics;
	class GraphicsResource;
	class GraphicsMgrRaw;

	/**
	 * A resource used in rendering that might need to create backend-specific resources.
	 *
	 * Any backend specific resources are created lazily, and the Resource keeps track of
	 * potentially multiple such resources for multiple backends.
	 *
	 * Each backend is allowed to store one pointer to additional data. The Resource class provides
	 * two interfaces for this, one with the suffix "Raw", that is not type-safe and not usable from
	 * Storm, and one without the suffix that operates on GraphicsResource objects exclusively.
	 */
	class Resource : public ObjectOn<Render> {
		STORM_ABSTRACT_CLASS;
	public:
		// Create.
		STORM_CTOR Resource();

		// Cleanup function for backend-specific data.
		typedef void (*Cleanup)(void *);

		// Destroy the resource for a particular Graphics backend.
		void STORM_FN destroy(Graphics *g);

		// Get a resource for a particular backend.
		GraphicsResource *STORM_FN forGraphics(Graphics *g);
		void *forGraphicsRaw(Graphics *g);

	protected:
		// Called whenever a resource needs to be created.
		virtual void create(GraphicsMgrRaw *g, void *&data, Cleanup &cleanup);

		// Called whenever a resource needs to be updated.
		virtual void update(GraphicsMgrRaw *g, void *data);

		// Called whenever something changed in this resource, and the underlying objects should be updated.
		void STORM_FN needUpdate();

		// Called whenever something changed that makes it necessary to re-create the resource when it is rendered again.
		void STORM_FN recreate();

	private:
		// Storage of elements.
		struct Element {
			// The stored pointer (GC:d).
			void *data;

			// Cleanup function (not GC:d).
			Cleanup clean;

			// Number of references. If zero, this element is unused. Tagged in the MSB if needs to be updated.
			Nat refs;
		};

		// GcType for Element.
		static const GcType elemType;

		// First element's data.
		UNKNOWN(PTR_GC) void *firstData;

		// First element's cleanup.
		UNKNOWN(PTR_NOGC) Cleanup firstClean;

		// First element's references.
		Nat firstRefs;

		// Offset of the first element (i.e. the real index of the first element).
		Nat offset;

		// Any remaining elements.
		GcArray<Element> *more;

		// Get an element. Returns number of references. Optionally increases the refcount if the element exists.
		Nat get(Nat id, void *&ptr, Bool addRef);

		// Set an element. Possibly creates it.
		Nat set(Nat id, void *ptr, Cleanup clean, Bool addRef);

		// Clear an element (i.e. decrementing the refcount).
		void clear(Nat id);

		// Resize the storage.
		void resize(Nat rangeMin, Nat rangeMax);

		// Shrink the storage to fit.
		void shrink();
	};


}
