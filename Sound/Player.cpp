#include "stdafx.h"
#include "Player.h"
#include "Audio.h"
#include "Exception.h"

namespace sound {

	const Nat Player::bufferPartTime = 1;
	const Nat Player::bufferParts = 3;

#ifdef SOUND_DX
	const Nat Player::sampleDepth = sizeof(float);
#endif
#ifdef SOUND_AL
	const Nat Player::sampleDepth = sizeof(short);
#endif

	const GcType Player::partInfoType = {
		GcType::tArray,
		null,
		null,
		sizeof(Player::PartInfo),
		0,
		{}
	};

	Player::Player(Sound *src) :
		src(src), buffer(), //event(null),
		lastFilled(0), bufferPlaying(false),
		fVolume(1.0f), tmpBuffer(null) {

		partInfo = runtime::allocArray<PartInfo>(engine(), &partInfoType, bufferParts);
		finishEvent = new (this) Event();

		// finishEvent.set();
		freq = src->sampleFreq();
		channels = src->channels();

		if (channels > 2)
			WARNING(L"More than two channels may not produce expected results.");

		initBuffer();

		audioMgr(engine())->addPlayer(this);
		fill();
	}

	Player::~Player() {
		// Do not close 'src' as it might have already been destroyed.
		src = null;
		close();
	}

	void Player::close() {
		if (buffer) {
			AudioMgr *mgr = audioMgr(engine());
			mgr->removePlayer(this);
			destroyBuffer();
			buffer = SoundBuffer();
		}

		if (src) {
			src->close();
			src = null;
		}
	}

	void Player::volume(Float to) {
		fVolume = to;

		if (buffer)
			bufferVolume(to);
	}

	Float Player::volume() {
		return fVolume;
	}

	void Player::play() {
		if (buffer) {
			bufferPlay();
			finishEvent->clear();
			bufferPlaying = true;
		}
	}

	void Player::pause() {
		if (buffer) {
			bufferPlaying = false;
			bufferStop();
			// TODO: This will not resume playback exactly where we paused.
		}
	}

	void Player::stop() {
		if (buffer) {
			bufferPlaying = false;
			bufferStop();
			finishEvent->set();

			// Reset position if possible.
			if (src->seek(0))
				fill();
		}
	}

	Bool Player::playing() {
		return bufferPlaying;
	}

	void Player::wait() {
		finishEvent->wait();
	}

	void Player::waitUntil(Duration t) {
		while (bufferPlaying) {
			Duration at = time();

			if (at >= t)
				break;

			Duration r = t - at;
			if (r > time::ms(400)) {
				// Long interval, yield to other threads.
				os::UThread::sleep(nat(r.inMs() - 100));
			} else {
				// Short interval left, poll as much as we can.
				os::UThread::leave();
			}
		}
	}

	Nat Player::next(nat part) {
		if (++part < bufferParts)
			return part;
		else
			return 0;
	}

	void Player::fill() {
		for (nat i = 0; i < bufferParts; i++)
			fill(i);
	}

	/**
	 * Backend specific code.
	 */

#ifdef SOUND_DX

	void Player::initBuffer() {
		WAVEFORMATEX fmt = {};
		fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		fmt.nChannels = channels;
		fmt.nSamplesPerSec = freq;
		fmt.nBlockAlign = channels * sampleDepth;
		fmt.nAvgBytesPerSec = freq * fmt.nBlockAlign;
		fmt.wBitsPerSample = sampleDepth * 8;
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
		AudioMgr *mgr = audioMgr(engine());
		if (!mgr->device())
			return;

		HRESULT r = mgr->device()->CreateSoundBuffer(&desc, &b, NULL);
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
				n[i].dwOffset = i * partSize();
				n[i].hEventNotify = event.v();
			}

			r = notify->SetNotificationPositions(bufferParts, n);
		}

		if (FAILED(r))
			throw SoundInitError();

		::release(notify);
		::release(b);
	}

	void Player::destroyBuffer() {
		::release(buffer);
		CloseHandle(event.v());
		event = Handle();
	}

	void Player::bufferVolume(float to) {
		// TODO? Transform the 'to' somehow first? Values below ~0.5 are almost not hearable.
		// Re-scale 'to' from [0 - 1] to [DSBVOLUME_MIN - DSBVOLUME_MAX].
		Float v = DSBVOLUME_MIN + to * (Float(DSBVOLUME_MAX) - Float(DSBVOLUME_MIN));
		buffer->SetVolume(LONG(v));
	}

	void Player::bufferPlay() {
		buffer->Play(0, 0, DSBPLAY_LOOPING);
	}

	void Player::bufferStop() {
		buffer->Stop();
	}

	Duration Player::time() {
		DWORD pos = 0;
		buffer->GetCurrentPosition(&pos, NULL);

		nat part = pos / partSize();
		nat64 sample = partInfo->v[part].sample;
		if (!partInfo->v[part].afterEnd)
			sample += (pos % partSize()) / sampleSize();

		sample *= 1000000;
		sample /= freq;

		return time::us(sample);
	}

	void Player::onNotify() {
		DWORD play;
		buffer->GetCurrentPosition(&play, NULL);
		nat playPart = play / partSize();

		if (partInfo->v[playPart].afterEnd) {
			stop();
		} else {
			for (nat at = next(lastFilled); at != playPart; at = next(at))
				fill(at);
		}
	}

	void Player::fill(nat part) {
		partInfo->v[part].sample = src->tell();
		partInfo->v[part].afterEnd = false;

		void *buf1 = null, *buf2 = null;
		DWORD size1 = 0, size2 = 0;
		buffer->Lock(partSize() * part, partSize(), &buf1, &size1, &buf2, &size2, 0);

		partInfo->v[part].afterEnd |= fill(buf1, size1);
		partInfo->v[part].afterEnd |= fill(buf2, size2);

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
				sound::Buffer r = src->read(size - at);
				memcpy(to + at, r.dataPtr(), r.filled()*sizeof(float));
				at += r.filled();
			} else {
				afterEnd = true;
				for (; at < size; at++)
					to[at] = 0.0f;
			}
		}

		return afterEnd;
	}

#endif
#ifdef SOUND_AL

	/**
	 * NOTE: In OpenAL the terminology is a bit different compared to DirectSound.
	 *
	 * A sound buffer is a sound source in OpenAL.
	 * A buffer part is a buffer in OpenAL.
	 *
	 * We are using the DirectSound terminology here to keep the naming consistent between the
	 * different backends.
	 */

	void Player::initBuffer() {
		ALuint source = 0;
		alGenSources(1, &source);
		buffer = source;

		if (!buffer)
			throw SoundInitError();

		alSourcef(source, AL_PITCH, 1);
		alSourcef(source, AL_GAIN, 1);
		alSource3f(source, AL_POSITION, 0, 0, 0);
		alSource3f(source, AL_VELOCITY, 0, 0, 0);
		alSourcei(source, AL_LOOPING, AL_FALSE);

		// Create one buffer for each part.
		for (Nat i = 0; i < partInfo->count; i++) {
			ALuint buf = 0;
			alGenBuffers(1, &buf);
			if (!buf)
				throw SoundInitError();

			partInfo->v[i].alBuffer = buf;
		}

		if (!tmpBuffer)
			tmpBuffer = malloc(partSize());
	}

	void Player::destroyBuffer() {
		free(tmpBuffer);
		tmpBuffer = null;

		ALuint source = buffer;
		alDeleteSources(1, &source);

		// Destroy the buffers as well.
		for (Nat i = 0; i < partInfo->count; i++) {
			ALuint buf = (ALuint)partInfo->v[i].alBuffer;
			alDeleteBuffers(1, &buf);
		}
	}

	void Player::bufferVolume(float to) {
		alSourcef(buffer, AL_GAIN, to);
	}

	void Player::bufferPlay() {
		alSourcePlay(buffer);
	}

	void Player::bufferStop() {
		alSourceStop(buffer);

		// Unqueue all buffers.
		for (Nat i = 0; i < partInfo->count; i++) {
			ALuint buf = (ALuint)partInfo->v[i].alBuffer;
			alSourceUnqueueBuffers(buffer, 1, &buf);
		}
	}

	static ALint getSource(ALuint source, ALenum param) {
		ALint result = 0;
		alGetSourcei(source, param, &result);
		return result;
	}

	Duration Player::time() {
		// TODO: Handle paused state as well.
		if (!playing())
			return time::us(0);

		// 'sample' is the samples in the processed buffers and the current buffer.
		Word sample = getSource(buffer, AL_SAMPLE_OFFSET);

		Nat part = next(lastFilled);
		sample += partInfo->v[part].sample;

		sample *= 1000000;
		sample /= freq;

		return time::us(sample);
	}

	void Player::onNotify() {
		ALint processed = getSource(buffer, AL_BUFFERS_PROCESSED);
		time();

		// Anything to do?
		if (processed <= 0)
			return;

		// Fill any processed parts.
		for (ALint i = 0; i < processed; i++) {
			nat part = next(lastFilled);

			// At the end of the stream?
			if (partInfo->v[part].afterEnd)
				stop();

			// We need to unqueue the buffer before we can fill it.
			ALuint buf = (ALuint)partInfo->v[part].alBuffer;
			alSourceUnqueueBuffers(buffer, 1, &buf);

			// Then fill and queue again. Updates 'lastFilled'.
			fill(part);
		}

		// See if we were too slow refilling the buffer.
		if (bufferPlaying && getSource(buffer, AL_SOURCE_STATE) == AL_STOPPED) {
			// Start playing again now that we have queued some more data.
			alSourcePlay(buffer);
		}
	}

	void Player::fill(nat part) {
		partInfo->v[part].sample = src->tell();
		partInfo->v[part].afterEnd = fill(tmpBuffer, partSize());

		ALuint partBuffer = partInfo->v[part].alBuffer;
		alBufferData(partBuffer, AL_FORMAT_STEREO16, tmpBuffer, partSize(), freq);
		alSourceQueueBuffers(buffer, 1, &partBuffer);

		lastFilled = part;
	}

	inline float clamp(float v, float max, float min) {
		return std::max(std::min(v, max), min);
	}

	inline short convert(float src) {
		return short(clamp(src * 32767, 32767, -32768));
	}

	bool Player::fill(void *buf, nat size) {
		bool afterEnd = false;
		short *to = (short *)buf;

		nat at = 0;
		size /= sizeof(short);

		while (at < size) {
			if (src->more()) {
				sound::Buffer r = src->read(size - at);
				for (Nat i = 0; i < r.filled(); i++)
					to[at++] = convert(r[i]);
			} else {
				afterEnd = true;
				for (; at < size; at++)
					to[at] = 0;
			}
		}

		return afterEnd;
	}

#endif
}
