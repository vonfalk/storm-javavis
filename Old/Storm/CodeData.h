#pragma once
#include "Utils/Function.h"
#include "Code/Listing.h"
#include "Shared/Auto.h"
#include "Shared/TObject.h"
#include "Thread.h"

namespace storm {
	STORM_PKG(core.asm);

	/**
	 * Data segments appended to the code.
	 * TODO: Expose to Storm properly!
	 */
	class CodeData : public ObjectOn<Compiler> {
		STORM_CLASS;
	public:

		// Add some data, get the label that will be used.
		void add(code::Label lbl, const Fn<void, code::Listing &> &fn, Par<Object> keepAlive = Par<Object>());

		// Output all data. (check out operator << below also).
		void output(code::Listing &to) const;

	private:
		struct Item {
			// Where to store the data?
			code::Label label;

			// Generate code
			Fn<void, code::Listing &> generate;

			// Object to keep alive.
			Auto<Object> keepAlive;
		};

		// Store everything...
		vector<Item> data;
	};

	inline code::Listing operator <<(code::Listing &to, const CodeData &src) {
		src.output(to);
		return to;
	}

}
