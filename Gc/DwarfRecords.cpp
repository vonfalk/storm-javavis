#include "stdafx.h"
#include "DwarfRecords.h"

namespace storm {

	/**
	 * Records
	 */

	void Record::setLen(void *end) {
		char *from = (char *)(void *)this;
		char *to = (char *)end;
		length = to - (from + 4);
	}

	void FDE::setCie(CIE *to) {
		char *me = (char *)&cieOffset;
		cieOffset = me - (char *)to;
	}

}
