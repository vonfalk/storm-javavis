#include "stdafx.h"
#include "Random.h"
#include "Timing.h"
#include "Utils/Lock.h"
#include <stdlib.h>

namespace storm {

	// Lock for the random number generator.
	static util::Lock randomLock;

	// Maximum random number + 1
	static const Word randMax = Word(RAND_MAX) + 1;

	// Get a random number (like rand).
	static Nat random() {
		util::Lock::L z(randomLock);

		static bool initialized = false;
		if (!initialized) {
			Moment moment;
			srand((unsigned int)moment.v);
			initialized = true;
		}

		return ::rand();
	}

	Int rand(Int min, Int max) {
		Nat nr = random() % Nat(max - min);
		return Int(nr) + min;
	}

	Nat rand(Nat min, Nat max) {
		Nat nr = random() % (max - min);
		return nr + min;
	}

	Float rand(Float min, Float max) {
		double nr = double(random()) / double(randMax);
		return Float(nr * (double(max) - double(min)) + double(min));
	}

}
