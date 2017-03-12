#include "stdafx.h"
#include "LibData.h"
#include "Audio.h"

namespace sound {

	static const Nat entries = 1;

	static inline GcArray<void *> *data(Engine &e) {
		return (GcArray<void *> *)e.data();
	}

	AudioMgr *&audioMgrData(Engine &e) {
		return (AudioMgr *&)data(e)->v[0];
	}

}

void *createLibData(storm::Engine &e) {
	using namespace sound;
	return runtime::allocArray<void *>(e, &pointerArrayType, sound::entries);
}

void destroyLibData(void *data) {
	using namespace sound;

	GcArray<void *> *d = (GcArray<void *> *)data;
	AudioMgr *audio = (AudioMgr *)d->v[0];
	if (audio)
		audio->terminate();
}
