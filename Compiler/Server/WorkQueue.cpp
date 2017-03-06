#include "stdafx.h"
#include "WorkQueue.h"
#include "Server.h"
#include "OS/UThread.h"

namespace storm {
	namespace server {


		WorkItem::WorkItem(File *file) : file(file) {}

		Range WorkItem::run(WorkQueue *q) {
			return Range();
		}

		Bool WorkItem::merge(WorkItem *o) {
			return runtime::typeOf(this) == runtime::typeOf(o)
				&& file == o->file;
		}

		/**
		 * Work queue.
		 */

		WorkQueue::WorkQueue(Server *callbackTo) : callbackTo(callbackTo), running(false), quit(false) {
			event = new (this) Event();
			work = new (this) Array<WorkItem *>();
		}

		void WorkQueue::start() {
			if (running)
				return;
			running = true;
			quit = false;

			os::FnStackParams<2> params;
			params.add(this);
			os::UThread::spawn(address(&WorkQueue::workerMain), true, params);
		}

		void WorkQueue::stop() {
			if (!running)
				return;
			quit = true;
			event->set();

			while (running)
				os::UThread::leave();
		}

		void WorkQueue::poke() {
			startWork = Moment() + time::ms(idleTime);
		}

		void WorkQueue::post(WorkItem *item) {
			// Linear search is good enough as we do not expect more than ~10 events in the queue at any given time.
			bool found = false;
			for (Nat i = 0; i < work->count(); i++) {
				if (work->at(i)->merge(item)) {
					found = true;
					break;
				}
			}

			if (!found)
				work->push(item);
			event->set();
		}

		void WorkQueue::workerMain() {
			poke();

			while (!quit) {
				event->wait();

				if (work->empty()) {
					event->clear();
					continue;
				}

				while (Moment() < startWork)
					os::UThread::sleep(idleTime / 2);

				Array<WorkItem *> *work = this->work;
				event->clear();
				this->work = new (this) Array<WorkItem *>();

				for (Nat i = 0; i < work->count(); i++) {
					callbackTo->runWork(work->at(i));
				}

				poke();
			}

			running = false;
		}

	}
}
