#include "stdafx.h"
#include "Player.h"
#include "Audio.h"

namespace sound {

	// Buffer time in seconds.
	static const nat bufferPartTime = 1;
	static const nat bufferParts = 3;

	Player::Player(Par<Sound> src) : src(src), buffer(null), event(null), lastFilled(0) {
		nat freq = src->sampleFreq();
		channels = src->channels();
		partSize = sizeof(float) * bufferPartTime * freq;
		bufferSize = partSize * bufferParts;

		if (channels > 2)
			WARNING(L"More than two channels may not produce expected results.");

		WAVEFORMATEX fmt = {};
		fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		fmt.nChannels = channels;
		fmt.nSamplesPerSec = freq;
		fmt.nBlockAlign = channels * sizeof(float);
		fmt.nAvgBytesPerSec = freq * fmt.nBlockAlign;
		fmt.wBitsPerSample = sizeof(float) * 8;
		fmt.cbSize = 0;

		DSBUFFERDESC desc = {
			sizeof(desc),
			DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GLOBALFOCUS,
			bufferSize,
			0,
			&fmt,
			DS3DALG_DEFAULT,
		};

		IDirectSoundBuffer *b = null;
		Auto<AudioMgr> mgr = audioMgr(engine());
		if (!mgr->ds())
			return;

		HRESULT r = mgr->ds()->CreateSoundBuffer(&desc, &b, NULL);
		if (SUCCEEDED(r)) {
			r = b->QueryInterface(IID_IDirectSoundBuffer8, (void **)&buffer);
		}

		IDirectSoundNotify8 *notify = null;
		if (SUCCEEDED(r)) {
			r = buffer->QueryInterface(IID_IDirectSoundNotify8, (void **)&notify);
		}

		if (SUCCEEDED(r)) {
			event = CreateEvent(NULL, TRUE, FALSE, NULL);

			// Set up notifications. One in the beginning of each section of the buffer.
			DSBPOSITIONNOTIFY n[bufferParts];
			for (nat i = 0; i < bufferParts; i++) {
				n[i].dwOffset = i * partSize;
				n[i].hEventNotify = event;
			}

			r = notify->SetNotificationPositions(bufferParts, n);
		}

		if (FAILED(r)) {
			WARNING(L"Failed to create a sound buffer.");
		} else {
			mgr->addPlayer(this, event);
			fill();
		}

		::release(notify);
		::release(b);
	}

	Player::~Player() {
		Auto<AudioMgr> mgr = audioMgr(engine());
		mgr->removePlayer(this);
		::release(buffer);
		CloseHandle(event);
	}

	void Player::play() {
		if (buffer)
			buffer->Play(0, 0, DSBPLAY_LOOPING);
	}

	static nat next(nat part) {
		if (++part < bufferParts)
			return part;
		else
			return 0;
	}

	void Player::onNotify() {
		DWORD play;
		buffer->GetCurrentPosition(&play, NULL);
		nat playPart = play / partSize;

		for (nat at = next(lastFilled); at != playPart; at = next(at))
			fill(at);
	}

	void Player::fill() {
		for (nat i = 0; i < bufferParts; i++)
			fill(i);
	}

	void Player::fill(nat part) {
		void *buf1 = null, *buf2 = null;
		DWORD size1 = 0, size2 = 0;
		buffer->Lock(partSize * part, partSize, &buf1, &size1, &buf2, &size2, 0);

		fill(buf1, size1);
		fill(buf2, size2);

		buffer->Unlock(buf1, size1, buf2, size2);

		lastFilled = part;
	}

	void Player::fill(void *buf, nat size) {
		size /= sizeof(float);
		float *to = (float *)buf;
		nat at = 0;
		while (at < size) {
			if (src->more()) {
				at += src->read(SoundBuffer(to + at, size - at));
			} else {
				for (; at < size; at++)
					to[at] = 0.0f;
			}
		}
	}

}
