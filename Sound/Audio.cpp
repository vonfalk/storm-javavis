#include "stdafx.h"
#include "Audio.h"
#include "Player.h"
#include "LibData.h"

namespace sound {

	AudioMgr::AudioMgr() : wait(null), dsound(null) {
		players = new (this) WeakSet<Player>();

		if (FAILED(DirectSoundCreate8(NULL, &dsound, NULL))) {
			WARNING(L"Failed to initialize DirectSound.");
		} else {
			// This is fine since we do not do anything that depends on focus:
			// https://groups.google.com/forum/#!msg/microsoft.public.win32.programmer.directx.audio/a7QhlgNFBpg/w8lTd8_NRSEJ
			dsound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
		}
	}

	void AudioMgr::terminate() {
		if (wait) {
			wait->terminate();
			wait = null;
		}

		WeakSet<Player>::Iter i = players->iter();
		while (Player *p = i.next()) {
			p->close();
		}

		::release(dsound);
	}

	void AudioMgr::addPlayer(Player *player) {
		players->put(player);
	}

	void AudioMgr::removePlayer(Player *player) {
		players->remove(player);
	}

	static bool signaled(HANDLE event) {
		return WaitForSingleObjectEx(event, 0, FALSE) != WAIT_TIMEOUT;
	}

	void AudioMgr::notifyEvents() {
		WeakSet<Player>::Iter i = players->iter();
		while (Player *p = i.next()) {
			if (p->waitEvent() == Handle())
				continue;

			HANDLE event = p->waitEvent().v();
			if (signaled(event)) {
				ResetEvent(event);
				p->onNotify();
			}
		}
	}

	void AudioMgr::allEvents(vector<HANDLE> &all) {
		all.resize(2);
		WeakSet<Player>::Iter i = players->iter();
		while (Player *p = i.next()) {
			Handle event = p->waitEvent();
			if (event != Handle())
				all.push_back(event.v());
		}
	}

	AudioMgr *audioMgr(EnginePtr e) {
		AudioMgr *&d = audioMgrData(e.v);
		if (!d)
			d = new (e.v) AudioMgr();
		return d;
	}

	/**
	 * Waiting logic.
	 */

	AudioWait::AudioWait(Engine &e) : e(e), exit(false), working(false), notifyExit(null) {
		AudioMgr *m = audioMgr(e);
		m->wait = this;
	}

	AudioWait::~AudioWait() {
		CloseHandle(events[1]);
	}

	void AudioWait::init() {
		events.push_back(NULL);
		events.push_back(CreateEvent(NULL, TRUE, FALSE, NULL));
	}

	bool AudioWait::wait(os::IOHandle io) {
		return wait(io, INFINITE);
	}

	bool AudioWait::wait(os::IOHandle io, nat ms) {
		if (exit) {
			doExit();
			return false;
		}

		events[0] = io.v();
		Nat first = !io ? 1 : 0;
		Nat count = 2;
		if (!working) {
			AudioMgr *m = audioMgr(e);
			m->allEvents(events);
			count = events.size();
		}

		DWORD z = WaitForMultipleObjectsEx(count - first, &events[first], FALSE, ms, FALSE);
		ResetEvent(events[1]);

		if (exit) {
			doExit();
		}
		return !exit;
	}

	void AudioWait::signal() {
		SetEvent(events[1]);
	}

	void AudioWait::work() {
		AudioMgr *m = audioMgr(e);
		working = true;
		m->notifyEvents();
		working = false;
		doExit();
	}

	void AudioWait::doExit() {
		if (exit && !working && notifyExit) {
			notifyExit->up();
			notifyExit = null;
		}
	}

	void AudioWait::terminate() {
		os::Sema sync(0);
		notifyExit = &sync;

		exit = true;

		// Allow the thread to exit properly...
		signal();
		sync.down();
	}

	os::Thread spawnAudioThread(Engine &e) {
		return os::Thread::spawn(new AudioWait(e), runtime::threadGroup(e));
	}

}
