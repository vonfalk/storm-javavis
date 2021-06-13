#include "stdafx.h"
#include "Accelerators.h"

namespace gui {

	Accelerators::Accelerators() {
		data = new (this) Map<KeyChord, Accel>();
	}

	Accelerators::~Accelerators() {}

	void Accelerators::add(KeyChord chord, Fn<void> *call) {
		data->put(chord, Accel(call));
	}

	void Accelerators::remove(KeyChord chord) {
		data->remove(chord);
	}

	Bool Accelerators::dispatch(KeyChord chord) {
		Accel a = data->get(chord, Accel());
		if (!a.any())
			return false;

		a.call->call();
		return true;
	}

}
