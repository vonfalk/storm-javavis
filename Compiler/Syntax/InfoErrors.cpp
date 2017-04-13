#include "stdafx.h"
#include "InfoErrors.h"

namespace storm {
	namespace syntax {

		static const Nat failedMask  = 0x80000000;
		static const Nat shiftsMask  = 0x7FFF0000;
		static const Nat skippedMask = 0x0000FFFF;
		static const Nat shiftsShift = 16;

		InfoErrors::InfoErrors() : data(0) {}

		InfoErrors::InfoErrors(Nat data) : data(data) {}

		Bool InfoErrors::success() const {
			return (data & failedMask) == 0;
		}

		Bool InfoErrors::error() const {
			// Checks: success() == false || shifts() > 0 || skipped() > 0
			return data != 0;
		}

		Nat InfoErrors::shifts() const {
			return (data & shiftsMask) >> shiftsShift;
		}

		Nat InfoErrors::skipped() const {
			return data & skippedMask;
		}

		InfoErrors InfoErrors::operator +(InfoErrors e) const {
			Nat a = shifts() + e.shifts();
			Nat b = skipped() + e.skipped();

			return InfoErrors(max(a << shiftsShift, shiftsMask)
							| max(b, skippedMask));
		}

		Bool InfoErrors::operator <(InfoErrors e) const {
			return (shifts() + skipped())
				< (e.shifts() + e.skipped());
		}

		InfoErrors infoFailed() {
			return InfoErrors(failedMask);
		}

		InfoErrors infoSuccess() {
			return InfoErrors(0);
		}

		InfoErrors infoShifts(Nat shifts) {
			return InfoErrors((shifts << shiftsShift) & shiftsMask);
		}

		InfoErrors infoSkipped(Nat skipped) {
			return InfoErrors(skipped & skippedMask);
		}

		Nat InfoErrors::getData(InfoErrors e) {
			return e.data;
		}

		InfoErrors InfoErrors::fromData(Nat v) {
			return InfoErrors(v);
		}

		StrBuf *operator <<(StrBuf *to, InfoErrors e) {
			if (e.success()) {
				if (!e.error())
					*to << L"success";
				else
					*to << e.skipped() << L" chars skipped, " << e.shifts() << L" corrections";
			} else {
				*to << L"failed";
			}

			return to;
		}

	}
}
