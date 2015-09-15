#pragma once
#include "Shared/Handle.h"
#include "Code/Reference.h"

namespace storm {
	class CloneEnv;

	/**
	 * Use a handle which updates automagically.
	 */
	class RefHandle : public Handle {
	public:
		// Ctor.
		RefHandle(const code::Content &content);

		// Dtor.
		~RefHandle();

		// Set references.
		void destroyRef(const code::Ref &ref);
		void destroyRef();
		void deepCopyRef(const code::Ref &ref);
		void deepCopyRef();
		void createRef(const code::Ref &ref);
		void equalsRef();
		void equalsRef(const code::Ref &ref);
		void hashRef();
		void hashRef(const code::Ref &ref);

	private:
		// References.
		code::AddrReference *destroyUpdater;
		code::AddrReference *createUpdater;
		code::AddrReference *deepCopyUpdater;
		code::AddrReference *equalsUpdater;
		code::AddrReference *hashUpdater;
		const code::Content &content;
	};


}
