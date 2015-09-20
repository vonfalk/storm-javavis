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
		void toSRef();
		void toSRef(const code::Ref &ref);

		// Output magic.
		virtual void output(const void *object, StrBuf *to) const;

		// Is this a value?
		bool isValue;

	private:
		// References.
		code::AddrReference *destroyUpdater;
		code::AddrReference *createUpdater;
		code::AddrReference *deepCopyUpdater;
		code::AddrReference *equalsUpdater;
		code::AddrReference *hashUpdater;
		code::AddrReference *toSUpdater;
		const code::Content &content;

		// ToS function. Always takes a single pointer to the object, even if the type we're
		// describing implies an extra indirection. This is seen by the 'isValue' member set by
		// 'Type'.
		typedef Str *(CODECALL *ToS)(const void *o);
		ToS toS;
	};


}
