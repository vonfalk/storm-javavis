#pragma once
#include "Gui/Device.h"
#include "Gui/Surface.h"
#include "OS/Stack.h"
#include "OS/StackCall.h"

namespace gui {

	/**
	 * A device used for working around buggy devices.
	 *
	 * For example, a version of the Iris driver accidentally saved a pointer to a stack-allocated
	 * variable in a persistent data structure and read from it later. This caused us to crash since
	 * we swap and deallocate stacks from time to time.
	 *
	 * This device wraps another device and ensures that it is always executed on one particular thread.
	 */
	class StackDevice : public Device {
	public:
		// Create the wrap device.
		StackDevice(Device *wrap);

		// Destroy.
		~StackDevice();

		// Create a surface.
		virtual Surface *createSurface(Handle window);

		// Create a text manager compatible with this device.
		virtual TextMgr *createTextMgr();

		// Hardware acceleration?
		virtual bool isHardware() const { return wrap->isHardware(); }

		// Call a function on the stack here. Assumes it is a non-member function.
		template <class Result, int params>
		Result call(const void *function, os::FnCall<Result, params> &call) {
			// Are we running on "stack" already>
			size_t dummy;
			if (size_t(&dummy) >= size_t(stack.allocLow()) && size_t(&dummy) < size_t(stack.allocHigh())) {
				// Yes, just call the function.
				return call.call(function, false);
			} else {
				// No, switch to the thread.
				os::Lock::L z(lock);
				return os::stackCall(stack, function, call, false);
			}
		}

	private:
		// Wrapped device.
		Device *wrap;

		// Lock for using the stack, in case we have multiple uthreads.
		os::Lock lock;

		// Stack.
		os::Stack stack;
	};

	/**
	 * Surface for wrapping calls on a particular stack.
	 */
	class StackSurface : public Surface {
	public:
		// Create.
		StackSurface(StackDevice *device, Surface *wrap);

		// Destroy.
		~StackSurface();

		// Create a graphics object.
		virtual WindowGraphics *createGraphics(Engine &e);

		// Resize this surface.
		virtual void resize(Size size, Float scale);

		// Try to present this surface directly from the Render thread.
		virtual PresentStatus present(bool waitForVSync);

		// Called from the UI thread when "present" returned "repaint".
		// Default implementation provided as all subclasses do not need this operation.
		virtual void repaint(RepaintParams *params);

	private:
		// Owner.
		StackDevice *owner;

		// Wrapping.
		Surface *wrap;
	};


}
