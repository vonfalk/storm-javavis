#pragma once
#include "Core/EnginePtr.h"
#include "Core/WeakSet.h"
#include "Handle.h"
#include "Player.h"

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

		// Get the direct sound interface.
		inline IDirectSound8 *ds() { return dsound; }

		// Add/remove a player to be notified about its events.
		void addPlayer(Player *player);
		void removePlayer(Player *player);

	private:
		friend class AudioWait;

		// AudioWait class.
		AudioWait *wait;

		// DirectSound device.
		IDirectSound8 *dsound;

		// All players associated with the audio manager.
		WeakSet<Player> *players;

		// Notify events for all players here.
		void notifyEvents();

		// Get a list of all events that provide us with wake notifications.
		void allEvents(vector<HANDLE> &events);
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
		virtual bool wait(os::IOHandle io);
		virtual bool wait(os::IOHandle io, nat ms);
		virtual void signal();
		virtual void work();

		// Terminate this thread.
		void terminate();

	private:
		Engine &e;

		// All event variables to be waited for. Index 0 is always present and used for the IO
		// handle in the wait functions. Index 1 is also always present and represents the event
		// that is signaled by 'signal' above. The others originate from events signaled by
		// DirectSound to indicate the need for refilling audio buffers.
		// All events here have a manual reset.
		vector<HANDLE> events;

		// Shall we exit?
		bool exit;

		// Are we currently processing notifications?
		bool working;

		// Notify termination completed.
		os::Sema *notifyExit;

		// Notify exit if feasible.
		void doExit();
	};

}
