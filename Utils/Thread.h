#pragma once

#include "Function.h"

namespace util {

	class Thread : NoCopy {
	public:
		Thread();
		~Thread();

		inline bool isRunning() const { return running != null; };

		class Control {
		public:
			inline Thread &thread() { return t; };
			inline void end() { t.running = null; };
			inline bool run() { return !t.end; };

			friend class Thread;
		protected:
			Control(Thread &t) : t(t) {};

			Thread &t;
		};

		bool start(const Function<void, Thread::Control &> &fn);

		void stop();
		void stopWait();

		bool sameAsCurrent();
	private:
		//CWinThread *running;
		void *running;
		bool end;

		// The function to start
		Function<void, Thread::Control &> fn;

		static UINT threadProc(LPVOID param);
	};


}
