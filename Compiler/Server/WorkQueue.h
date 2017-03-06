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

		class File;
		class Server;
		class WorkQueue;

		/**
		 * Work items. Subclass this class to provide custom messages.
		 *
		 * All work items which are considered equal are possibly reduced into one.
		 */
		class WorkItem : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create. Give a file that should possible be updated on completion.
			STORM_CTOR WorkItem(File *file);

			// File this item is regarding.
			File *file;

			// Execute this work. Gives a range to be updated.
			virtual Range STORM_FN run(WorkQueue *q);

			// Merge another work item with this one. Return true if the merge was possible.
			virtual Bool STORM_FN merge(WorkItem *o);
		};

		/**
		 * A queue of work to be delayed until the user is idle.
		 */
		class WorkQueue : public ObjectOn<Compiler> {
			STORM_CLASS;
		public:
			// Create.
			STORM_CTOR WorkQueue(Server *callbackTo);

			// Start the UThread waiting for inactivity.
			void STORM_FN start();

			// Stop the UThread waiting for inactivity.
			void STORM_FN stop();

			// Notify the wait about user activity.
			void STORM_FN poke();

			// Post a message.
			void STORM_FN post(WorkItem *item);

		private:
			// Dispatch callbacks to here.
			Server *callbackTo;

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

			// Work to do. Only contains unique instances of work-items.
			Array<WorkItem *> *work;

			// Main function in the worker thread.
			void CODECALL workerMain();
		};

	}
}
