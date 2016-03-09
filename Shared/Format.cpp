#include "stdafx.h"
#include "Format.h"
#include <iomanip>

namespace storm {

	static void toBytes(Word size, std::wostream &to) {
		const wchar *units[] = { L"byte", L"KB", L"MB", L"GB", L"TB" };

		if (size < 1024) {
			to << size << ' ' << units[0];
			return;
		}

		to << std::fixed << std::setprecision(2);

		double s = double(size);
		for (nat i = 0; i < ARRAY_COUNT(units) - 1; i++, s /= 1024) {
			if (s < 1024) {
				to << s << ' ' << units[i];
				return;
			}
		}

		to << s << ' ' << units[ARRAY_COUNT(units) - 1];
	}

	Str *toBytes(EnginePtr e, Word size) {
		std::wostringstream to;
		toBytes(size, to);
		return CREATE(Str, e.v, to.str());
	}

	String toBytes(Word size) {
		std::wostringstream to;
		toBytes(size, to);
		return to.str();
	}

}
