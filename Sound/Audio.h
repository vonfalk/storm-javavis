#pragma once
#include "Core/EnginePtr.h"
#include "Core/WeakSet.h"
#include "Handle.h"
#include "Player.h"
#include "Types.h"

namespace sound {
	class AudioWait;

	/**
	 * Management of audio playback.
	 */
	class AudioMgr : public ObjectOn<Audio> {
		STORM_CLASS;
	public:
		// Craete. Use 'audioMgr' instead.
		AudioMgr();

		// Terminate and clean up the audio thread.
		void terminate();

		// Add/remove a player to be notified about its events.
		void addPlayer(Player *player);
		void removePlayer(Player *player);

		// Get the sound device.
		inline SoundDevice device() { return soundDevice; }

	private:
		friend class AudioWait;

		// AudioWait class.
		AudioWait *wait;

		// Sound device and context.
		SoundDevice soundDevice;
		SoundContext soundContext;

		// All players associated with the audio manager.
		WeakSet<Player> *players;

		// Notify events for all players here.
		void notifyEvents();

		// Initialize the device.
		void init();

		// Destroy the device.
		void destroy();

#ifdef SOUND_DX

		// Get a list of all events that provide us with wake notifications.
		void allEvents(vector<HANDLE> &events);

#endif
#ifdef SOUND_AL

		// See if any playback is performed at the moment.
		bool anyPlayback();

#endif
	};

	// Get the global AudioMgr instance.
	AudioMgr *STORM_FN audioMgr(EnginePtr e) ON(Audio);

	/**
	 * Custom waiting.
	 */
	class AudioWait : public os::ThreadWait {
	public:
		AudioWait(Engine &e);
		~AudioWait();

		virtual void init();
		virtual bool wait(os::IOHandle &io);
		virtual bool wait(os::IOHandle &io, nat ms);
		virtual void signal();
		virtual void work();

		// Terminate this thread.
		void terminate();

	private:
		Engine &e;

#ifdef SOUND_DX

		// All event variables to be waited for. Index 0 is always present and used for the IO
		// handle in the wait functions. Index 1 is also always present and represents the event
		// that is signaled by 'signal' above. The others originate from events signaled by
		// DirectSound to indicate the need for refilling audio buffers.
		// All events here have a manual reset.
		vector<HANDLE> events;

#endif
#ifdef SOUND_AL

		// IO wait object.
		os::IOCondition ioWait;

#endif

		// Shall we exit?
		bool exit;

		// Are we currently processing notifications?
		bool working;

		// Notify termination completed.
		os::Sema *notifyExit;

		// Notify exit if feasible.
		void doExit();

		// Destroy any backend specific resources.
		void destroy();
	};

}
