#pragma once

#include "Utils/HashMap.h"
#include "Utils/InlineSet.h"
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
		void addLightRef(nat id);
		void removeLightRef(nat id);

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
		struct SourceInfo : public util::SetMember<SourceInfo> {
			// Number of light references.
			nat lightRefs;

			// All heavy references.
			RefSet refs;

			// Currently active content.
			ContentInfo *content;

			// Name of this SourceInfo.
			String name;

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

		// index->SourceInfo
		typedef hash_map<nat, SourceInfo *> SourceMap;
		SourceMap sources;

		// Content*->ContentInfo
		typedef hash_map<Content *, ContentInfo *> ContentMap;
		ContentMap contents;

		// First possible free index.
		nat firstFreeIndex;

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
	};

}
