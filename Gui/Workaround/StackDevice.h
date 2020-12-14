#pragma once
#include "Workaround.h"
#include "Gui/Device.h"
#include "Gui/Surface.h"
#include "OS/Stack.h"
#include "OS/StackCall.h"

namespace gui {

	/**
	 * A workaround for buggy device drivers.
	 *
	 * For example, a version of the Iris driver accidentally saved a pointer to a stack-allocated
	 * variable in a persistent data structure and read from it later. This caused us to crash since
	 * we swap and deallocate stacks from time to time.
	 *
	 * This device wraps another device and ensures that it is always executed on one particular thread.
	 */
	class StackWorkaround : public SurfaceWorkaround {
	public:
		// Create.
		StackWorkaround(SurfaceWorkaround *prev);

		// Destroy.
		~StackWorkaround();

		// Call a function on the stack here. Assumes it is a non-member function.
		template <class Result, int params>
		Result call(const void *function, os::FnCall<Result, params> &call) {
			// Are we running on "stack" already>
			size_t dummy;
			if (size_t(&dummy) >= size_t(stack.low()) && size_t(&dummy) < size_t(stack.high())) {
				// Yes, just call the function.
				return call.call(function, false);
			} else {
				// No, switch to the thread.
				os::Lock::L z(lock);
				return os::stackCall(stack, function, call, false);
			}
		}

	protected:
		// Apply here.
		virtual Surface *applyThis(Surface *to);

	private:
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
		StackSurface(StackWorkaround *device, Surface *wrap);

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
		StackWorkaround *owner;

		// Wrapping.
		Surface *wrap;
	};


}
