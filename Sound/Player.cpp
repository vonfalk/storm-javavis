#include "stdafx.h"
#include "Player.h"
#include "Audio.h"

namespace sound {

	// Buffer time in seconds.
	static const nat bufferPartTime = 1;
	static const nat bufferParts = 3;

	Player::Player(Par<Sound> src) :
		src(src), buffer(null), event(null),
		lastFilled(0), bufferPlaying(false),
		fVolume(1.0f) {

		finishEvent.set();
		freq = src->sampleFreq();
		channels = src->channels();
		sampleSize = sizeof(float) * channels;
		partSize = sampleSize * bufferPartTime * freq;
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

	void Player::volume(Float to) {
		fVolume = to;
		// TODO? Transform the 'to' somehow first? Values below ~0.5 are almost not hearable.
		// Re-scale 'to' from [0 - 1] to [DSBVOLUME_MIN - DSBVOLUME_MAX].
		Float v = DSBVOLUME_MIN + to * (Float(DSBVOLUME_MAX) - Float(DSBVOLUME_MIN));
		if (buffer) {
			buffer->SetVolume(LONG(v));
		}
	}

	Float Player::volume() {
		return fVolume;
	}

	void Player::play() {
		if (buffer) {
			buffer->Play(0, 0, DSBPLAY_LOOPING);
			finishEvent.clear();
			bufferPlaying = true;
		}
	}

	void Player::pause() {
		if (buffer) {
			bufferPlaying = false;
			buffer->Stop();
		}
	}

	void Player::stop() {
		if (buffer) {
			bufferPlaying = false;
			buffer->Stop();
			finishEvent.set();

			// Reset position.
			src->seek(0);
			fill();
		}
	}

	Bool Player::playing() {
		return bufferPlaying;
	}

	void Player::wait() {
		finishEvent.wait();
	}

	void Player::wait(Duration t) {
		while (bufferPlaying) {
			Duration at = time();

			if (at >= t)
				break;

			Duration r = at - t;
			if (r > ms(400)) {
				// Long interval, yeild to other threads.
				os::UThread::sleep(nat(r.inMs() - 100));
			} else {
				// Short interval left, poll as much as we can.
				os::UThread::leave();
			}
		}
	}

	Duration Player::time() {
		DWORD pos = 0;
		buffer->GetCurrentPosition(&pos, NULL);

		nat part = pos / partSize;
		nat64 sample = partSample[part] + (pos % partSize) / sampleSize;

		sample *= 1000000;
		sample /= freq;

		return us(sample);
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

		if (afterEnd[playPart]) {
			stop();
		} else {
			for (nat at = next(lastFilled); at != playPart; at = next(at))
				fill(at);
		}
	}

	void Player::fill() {
		for (nat i = 0; i < bufferParts; i++)
			fill(i);
	}

	void Player::fill(nat part) {
		partSample[part] = src->tell();
		afterEnd[part] = false;

		void *buf1 = null, *buf2 = null;
		DWORD size1 = 0, size2 = 0;
		buffer->Lock(partSize * part, partSize, &buf1, &size1, &buf2, &size2, 0);

		afterEnd[part] |= fill(buf1, size1);
		afterEnd[part] |= fill(buf2, size2);

		buffer->Unlock(buf1, size1, buf2, size2);

		lastFilled = part;
	}

	bool Player::fill(void *buf, nat size) {
		bool afterEnd = false;
		float *to = (float *)buf;
		nat at = 0;
		size /= sizeof(float);

		while (at < size) {
			if (src->more()) {
				at += src->read(SoundBuffer(to + at, size - at));
			} else {
				afterEnd = true;
				for (; at < size; at++)
					to[at] = 0.0f;
			}
		}

		return afterEnd;
	}

}
