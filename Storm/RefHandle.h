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

		enum OutputMode {
			outputNone,
			// Add directly to StrBuf. 'outputFn' is StrBuf *(CODECALL *)(StrBuf *, T)
			outputAdd,
			// ToS by reference. 'outputFn' is Str *(CODECALL *)(T *) always member fn.
			outputByRefMember,
			// ToS by reference. 'outputFn' is Str *(CODECALL *)(T *) always non-member.
			outputByRef,
			// ToS by value. 'outputFn' is Str *(CODECALL *)(T) always non-member.
			outputByVal,
		};

		void outputRef();
		void outputRef(const code::Ref &ref, OutputMode mode);

		// Output magic.
		virtual void output(const void *object, StrBuf *to) const;

	private:
		// References.
		code::AddrReference *destroyUpdater;
		code::AddrReference *createUpdater;
		code::AddrReference *deepCopyUpdater;
		code::AddrReference *equalsUpdater;
		code::AddrReference *hashUpdater;
		code::AddrReference *outputUpdater;
		const code::Content &content;

		// Function for output. Type depends on 'outputMode'.
		const void *outputFn;

		// Mode of 'outputFn'.
		OutputMode outputMode;
	};


}
