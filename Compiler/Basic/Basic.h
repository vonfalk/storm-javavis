#pragma once

namespace storm {
	namespace basic {
		STORM_PKG(lang.bs);

		/**
		 * Conventions used in this language:
		 *
		 * When some construct tells that it can return a reference, it is also able to
		 * return a pure value. Just pass a value type as the result when generating
		 * code and it shall be implemented so that it generates a value for you. On
		 * the other hand, if a construct tells you that it returns a value, it can
		 * not give you the reference, you have the responsibility as the caller to
		 * generate the reference from a temporary value if it is needed.
		 */

	}
}
