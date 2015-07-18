#include "stdafx.h"
#include "Audio.h"
#include "Player.h"

namespace sound {

	AudioMgr::AudioMgr() : wait(null), dsound(null) {
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_SPEED_OVER_MEMORY);

		if (FAILED(DirectSoundCreate8(NULL, &dsound, NULL))) {
			WARNING(L"Failed to initialize DirectSound.");
		} else {
			// This is fine since we do not do anything that depends on focus:
			// https://groups.google.com/forum/#!msg/microsoft.public.win32.programmer.directx.audio/a7QhlgNFBpg/w8lTd8_NRSEJ
			dsound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
		}
	}

	AudioMgr::~AudioMgr() {
		::release(dsound);

		CoUninitialize();
	}

	void AudioMgr::addPlayer(Player *player, HANDLE event) {
		if (wait)
			wait->addPlayer(player, event);
	}

	void AudioMgr::removePlayer(Player *player) {
		if (wait)
			wait->removePlayer(player);
	}

	void AudioMgr::terminate() {
		if (wait)
			wait->terminate();
	}

	AudioMgr *audioMgr(EnginePtr e) {
		LibData *d = e.v.data();
		if (!d->audio)
			d->audio = CREATE(AudioMgr, e.v);
		return d->audio.ret();
	}


	AudioWait::AudioWait(Engine &e) : exit(false) {
		Auto<AudioMgr> m = audioMgr(e);
		m->wait = this;
	}

	AudioWait::~AudioWait() {
		for (nat i = 0; i < events.size(); i++) {
			CloseHandle(events[i]);
		}
	}

	void AudioWait::init() {
		events.push_back(CreateEvent(NULL, TRUE, FALSE, NULL));
		players.push_back(null);
	}

	bool AudioWait::wait() {
		if (exit)
			return false;

		WaitForMultipleObjectsEx(events.size(), &events[0], FALSE, INFINITE, FALSE);
		ResetEvent(events[0]);

		return !exit;
	}

	bool AudioWait::wait(nat ms) {
		if (exit)
			return false;

		WaitForMultipleObjectsEx(events.size(), &events[0], FALSE, ms, FALSE);
		ResetEvent(events[0]);

		return !exit;
	}

	void AudioWait::signal() {
		SetEvent(events[0]);
	}

	void AudioWait::work() {
		notifyEvents();
	}

	void AudioWait::addPlayer(Player *player, HANDLE event) {
		players.push_back(player);
		events.push_back(event);
	}

	void AudioWait::removePlayer(Player *player) {
		// It is safe to alter these arrays here, we're not waiting for them as this has to be
		// executed on the thread that potentially does the waiting.
		for (nat i = 0; i < players.size(); i++) {
			if (players[i] == player) {
				players.erase(players.begin() + i);
				events.erase(events.begin() + i);
				return;
			}
		}
	}

	static bool signaled(HANDLE event) {
		return WaitForSingleObjectEx(event, 0, FALSE) != WAIT_TIMEOUT;
	}

	void AudioWait::notifyEvents() {
		for (nat i = 1; i < events.size(); i++) {
			if (signaled(events[i])) {
				ResetEvent(events[i]);
				players[i]->onNotify();
			}
		}
	}

	void AudioWait::terminate() {
		exit = true;

		// Allow the thread to exit properly...
		signal();
		Sleep(10);
	}

}
