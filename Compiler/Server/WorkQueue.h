#pragma once
#include "Core/Fn.h"
#include "Core/Array.h"
#include "Core/Event.h"
#include "Core/Timing.h"
#include "Compiler/Thread.h"
#include "Range.h"

namespace storm {
	namespace server {
		STORM_PKG(core.lang.server);

		/**
		 * Work items.
		 */
		class WorkItem {
			STORM_VALUE;
		public:
			STORM_CTOR WorkItem(Fn<void, Range> *fn, Range range);

			// Function to call.
			Fn<void, Range> *fn;

			// Desired range.
			Range range;
		};

		/**
		 * A queue of work to be delayed until the user is idle.
		 */
		class WorkQueue : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WorkQueue();

			// Start the UThread waiting for inactivity.
			void STORM_FN start();

			// Stop the UThread waiting for inactivity.
			void STORM_FN stop();

			// Notify the wait about user activity.
			void STORM_FN poke();

			// Post a message.
			void STORM_FN post(WorkItem item);

		private:
			// How much inactive time is to be passed before something works?
			enum {
				idleTime = 500
			};

			// Is the worker thread running?
			Bool running;

			// Quit the worker thread?
			Bool quit;

			// Event used to make the worker thread wait.
			Event *event;

			// First possible moment we are allowed to do something.
			Moment startWork;

			// Work to do.
			Array<WorkItem> *work;

			// Main function in the worker thread.
			void CODECALL workerMain();
		};

	}
}
