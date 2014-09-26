#include "StdAfx.h"
#include "FramerateTimer.h"

namespace util {

	FramerateTimer::Time::Time(FramerateTimer &t) : owner(t) {}

	FramerateTimer::Time::~Time() {
		owner.onInterval(util::Timestamp() - startTime);
	}


	FramerateTimer::FramerateTimer(const wchar_t *name) : name(name), currentSample(0) {}


	void FramerateTimer::onInterval(util::Timespan time) {
		samples[currentSample++] = time;
		if (currentSample == numSamples) {
			outputStats();
			currentSample = 0;
		}
	}

	void FramerateTimer::outputStats() {
		util::Timespan minTime = samples[0];
		util::Timespan maxTime = samples[0];
		util::Timespan sumTime = samples[0];

		for (nat i = 1; i < numSamples; i++) {
			sumTime += samples[i];
			limitMax(minTime, samples[i]);
			limitMin(maxTime, samples[i]);
		}

		sumTime = sumTime / numSamples;
		debugStream() << name << ": Min: " << minTime << ", max: " << maxTime << ", avg: " << sumTime << std::endl;
	}

}