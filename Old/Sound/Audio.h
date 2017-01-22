#pragma once
#include "OS/Thread.h"

namespace sound {

	class AudioWait;
	class Player;

	/**
	 * Management of audio playback.
	 */
	class AudioMgr : public ObjectOn<Audio> {
		STORM_CLASS;
	public:
		// Create. Use 'audioMgr' instead.
		AudioMgr();

		// Destroy.
		~AudioMgr();

		// Get the dsound interface.
		inline IDirectSound8 *ds() { return dsound; }

		// Add a Player to be notified about its events.
		void addPlayer(Player *player, HANDLE event);
		void removePlayer(Player *player);

		// Terminate the audio thread.
		void terminate();

	protected:
		// Allow any thread to delete us, as threading may not be available when we're removed.
		virtual void deleteMe() {
			delete this;
		}

	private:
		friend class AudioWait;

		// AudioWait class.
		AudioWait *wait;

		// DirectSound device.
		IDirectSound8 *dsound;
	};

	// Get the global AudioMgr instance.
	AudioMgr *STORM_ENGINE_FN audioMgr(EnginePtr e) ON(Audio);

	/**
	 * Custom waiting.
	 */
	class AudioWait : public os::ThreadWait {
	public:
		AudioWait(Engine &e);
		~AudioWait();

		virtual void init();
		virtual bool wait();
		virtual bool wait(nat ms);
		virtual void signal();
		virtual void work();

		// Add/remove player notifications. Only call from this thread.
		void addPlayer(Player *p, HANDLE event);
		void removePlayer(Player *p);

		// Terminate this thread.
		void terminate();

	private:
		// All event variables to be waited for. Index 0 is always present and represents the event
		// that is signaled by 'signal' above. The others originate from events signaled by
		// DirectSound to indicate the need for refilling audio buffers.
		// All events here have a manual reset.
		vector<HANDLE> events;

		// Players to notify. The first element is always null.
		vector<Player *> players;

		// Shall we exit?
		bool exit;

		// Check any signaled events that needs to be notified.
		void notifyEvents();

	};

}
