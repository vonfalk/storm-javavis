#include "stdafx.h"
#include "WorkQueue.h"
#include "OS/UThread.h"

namespace storm {
	namespace server {

		WorkItem::WorkItem(Fn<void, Range> *fn, Range range) : fn(fn), range(range) {}

		WorkQueue::WorkQueue() : running(false), quit(false) {
			event = new (this) Event();
			work = new (this) Array<WorkItem>();
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
			startWork = Moment() + ms(idleTime);
		}

		void WorkQueue::post(WorkItem item) {
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

				Array<WorkItem> *work = this->work;
				event->clear();
				this->work = new (this) Array<WorkItem>();

				for (Nat i = 0; i < work->count(); i++) {
					const WorkItem &item = work->at(i);
					item.fn->call(item.range);
				}
			}

			running = false;
		}

	}
}
