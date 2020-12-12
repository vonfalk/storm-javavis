#pragma once
#include "Gui/Device.h"
#include "Gui/Surface.h"

namespace gui {

	/**
	 * A workaround-entry that will wrap a surface appropriately. The function "glWorkarounds" below
	 * will return an instance of this class to indicate if the implementation needs to wrap all
	 * surfaces in some workaround.
	 *
	 * The system expects that a single instance of this class is created for each GL context, or
	 * corresponding. Creating multiple instances for a single context might lead to issues.
	 */
	class SurfaceWorkaround : NoCopy {
	public:
		// Create, possibly supplying the previous fix to apply.
		SurfaceWorkaround(SurfaceWorkaround *prev);

		// Destroy.
		~SurfaceWorkaround();

		// Apply fixes to a surface.
		Surface *apply(Surface *to);

	protected:
		// Apply this layer to a surface.
		virtual Surface *applyThis(Surface *to) = 0;

	private:
		// Previous.
		SurfaceWorkaround *prev;
	};


	/**
	 * A device that applies a set of workarounds.
	 *
	 * Used to wrap workarounds that only need to be applied to surfaces without having to create a
	 * Device class for them as well.
	 */
	class WorkaroundDevice : public Device {
	public:
		// Create.
		WorkaroundDevice(Device *wrap, SurfaceWorkaround *workarounds);

		// Destroy.
		~WorkaroundDevice();

		// Create a surface.
		virtual Surface *createSurface(Handle window);

		// Create a text manager compatible with this device.
		virtual TextMgr *createTextMgr();

	private:
		// Wrapped device.
		Device *wrap;

		// Workaround.
		SurfaceWorkaround *workarounds;
	};

}
