#pragma once

#include "Utils/HashMap.h"
#include "Utils/InlineSet.h"
#include "Utils/Lock.h"
#include "Reference.h"

namespace code {

	class RefSource;

	/**
	 * This class is responsible for keeping track of all references within an arena. It has
	 * ownership of all added Content objects, and it traverses the reference graph to figure out
	 * which ones can be safely deleted. The RefSource that are destroyed are actually kept alive
	 * until they do not have any more references (as ID:s).
	 *
	 * TODO? Make sure everything is thread-safe?
	 *
	 * TODO? Prepare the reference manager for a lot of changes, and make it do a GC afterwards, instead
	 *  of checking dead objects always?
	 */
	class RefManager : NoCopy {
	public:
		RefManager();
		~RefManager();

		// Clear all data.
		void clear();

		// Invalid id.
		static const nat invalid = -1;

		// Source operations.
		nat addSource(RefSource *source, const String &title);
		void removeSource(nat id);
		void setContent(nat id, Content *content);

		// Content operations.
		void updateAddress(Content *content);
		void contentDestroyed(Content *content);

		// Get the source that is currently associated with the address at "addr". Returns "invalid" if none exists.
		// Works under the assumption that no sources refer overlapped chunks of memory.
		nat ownerOf(void *addr) const;

		// Get the current address of a source.
		void *address(nat id) const;
		// Get the name of RefSource "id".
		const String &name(nat id) const;

		// Reference counting used for the Ref class (light references).
		static void addLightRef(void *v);
		static void removeLightRef(void *v);

		// Other utilities for the Ref class.
		static void *address(void *v);
		static const String &name(void *v);
		static nat refId(void *v);

		// Note: not thread safe. Get the pointer for an ID. Calls 'addLightRef'.
		void *lightRefPtr(nat id);

		// Reference management for the Reference class.
		void *addReference(Reference *r, nat id);
		void removeReference(Reference *r, nat id);

		// PreShutdown. Optimization that skips any checks for unreachable islands, intended to
		// be used when we know that all references will be destroyed soon anyway.
		void preShutdown();

	private:
		struct ContentInfo;

		typedef util::InlineSet<Reference> RefSet;

		// Represents a RefSource. These are kept alive until they are deemed unreachable from
		// any live RefSource objects.
		// NOTE: This is implicitly shared by pointer to the Ref class to allow safe manipulations
		// from other threads than the owning thread. This is done to avoid needing to synchronize the
		// hash maps.
		struct SourceInfo : public util::SetMember<SourceInfo> {
			// Our ID.
			nat id;

			// Number of light references.
			nat lightRefs;

			// All heavy references.
			RefSet refs;

			// Currently active content.
			ContentInfo *content;

			// Name of this SourceInfo.
			String name;

			// Last known address.
			void *address;

			// Is the RefSource alive?
			bool alive;
		};

		typedef util::InlineSet<SourceInfo> SourceSet;

		// Extra data about a Content object.
		struct ContentInfo {
			// All Source objects that have this Content set at the moment.
			SourceSet sources;

			// The actual Content object. We own this pointer.
			Content *content;
		};

		// index->SourceInfo. Locked by 'sourceLock'
		typedef hash_map<nat, SourceInfo *> SourceMap;
		SourceMap sources;

		// Lock for 'sources'.
		mutable util::Lock sourceLock;

		// Content*->ContentInfo
		typedef hash_map<Content *, ContentInfo *> ContentMap;
		ContentMap contents;

		// First possible free index.
		nat firstFreeIndex;

		// Currently shutting down?
		bool shutdown;

		// Generate a new free id.
		nat freeId();

		// Get an address from a SourceInfo.
		static void *address(SourceInfo *info);

		// Detach content from a SourceInfo.
		void detachContent(SourceInfo *from);

		// Attach content to a SourceInfo.
		void attachContent(SourceInfo *to, Content *content);
		void attachContent(SourceInfo *to, ContentInfo *content);

		// Broadcast address updates.
		void broadcast(SourceInfo *to, void *address);
		void broadcast(ContentInfo *to, void *address);

		// Remove a source if it has zero references (or is free in other ways).
		void cleanSource(nat id, SourceInfo *source);

		// Get the SourceInfo from an id.
		SourceInfo *source(nat id) const;
		SourceInfo *sourceUnsafe(nat id) const;

		// Get the ContentInfo from a Content ptr.
		ContentInfo *content(const Content *ptr) const;
		ContentInfo *contentUnsafe(const Content *ptr) const;

		// See if any live SourceInfo is reachable.
		typedef set<SourceInfo *> ReachableSet;
		bool liveReachable(ReachableSet &reachable, SourceInfo *at) const;
	};

}
