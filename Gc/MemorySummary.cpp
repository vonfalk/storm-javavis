#include "stdafx.h"
#include "MemorySummary.h"

namespace storm {

	MemorySummary::MemorySummary()
		: objects(0), fragmented(0), bookkeeping(0), free(0), allocated(0), reserved(0) {}

	wostream &operator <<(wostream &to, const MemorySummary &o) {
		to << L"Memory summary:\n";
		to << L"Object bytes    : " << std::setw(10) << o.objects << L"\n";
		to << L"Fragmented bytes: " << std::setw(10) << o.fragmented << L"\n";
		to << L"Free bytes      : " << std::setw(10) << o.free << L"\n";
		to << L"Bookkeeping     : " << std::setw(10) << o.bookkeeping << L"\n";
		to << L"Allocated bytes : " << std::setw(10) << o.allocated << L"\n";
		to << L"Reserved bytes  : " << std::setw(10) << o.reserved << L"\n";
		return to;
	}

}
