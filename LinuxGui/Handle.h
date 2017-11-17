#pragma once
#include "Core/Hash.h"

namespace gui {

	/**
	 * Encapsulates a Win32 handle so that it can be properly understood by Storm.
	 */
	class Handle {
		STORM_VALUE;
	public:
		Handle() { value = null; }
		Handle(void *value) { value = value; }

		// Get various GObjects.
		inline GtkWidget *widget() const { return (GtkWidget *)value; }

		inline Bool STORM_FN operator ==(Handle o) const {
			return value == o.value;
		}

		inline Bool STORM_FN operator !=(Handle o) const {
			return value != o.value;
		}

		inline Nat STORM_FN hash() const {
			return ptrHash(value);
		}

	private:
		UNKNOWN(PTR_NOGC) void *value;
	};

}
