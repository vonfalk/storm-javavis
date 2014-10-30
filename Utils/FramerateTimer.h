#pragma once

#include "Timestamp.h"

namespace util {

	// A class designed to measure a large number of short intervals and report
	// any outstandingly large intervals. This is useful for measuring rendering times.
	// The class is used by creating a static instance of the FramerateTimer class
	// in the beginning of the function to be measured. Then an instance of FramerateTimer::Time
	// should be created so that its lifetime is during what is to be measured.
	//
	// Example:
	// static FramerateTimer timer(L"Rendering");
	// FramerateTimer::Time timerTime(timer);
	// 
	// This is what is done by the macro FRAMERATE_TIME(L"Rendering");
	class FramerateTimer : NoCopy {
	public:
		// Create the static class with a label. This label is used with outputs.
		FramerateTimer(const wchar_t *name);

		// Class to keep track of a single run.
		class Time : NoCopy {
		public:
			Time(FramerateTimer &t);
			~Time();
		private:
			FramerateTimer &owner;
			Timestamp startTime;
		};

	private:
		const wchar_t *name;

		// Called by ~Time when an interval has been elapsed.
		void onInterval(Timespan time);

		// Number of samples before output
		static const nat numSamples = 100;

		// Current sample.
		nat currentSample;

		// Sample record.
		Timespan samples[numSamples];

		// Output statistics.
		void outputStats();
	};

}

// Macros
#ifdef _DEBUG
#define FRAMERATE_TIME(X) \
	static util::FramerateTimer timer(X); \
	util::FramerateTimer::Time timerTime(timer)

#else
#define FRAMERATE_TIME(X)
#endif
