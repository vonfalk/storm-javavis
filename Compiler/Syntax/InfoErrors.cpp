#include "stdafx.h"
#include "InfoErrors.h"
#include "InfoNode.h"

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

		Bool InfoErrors::any() const {
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
			Nat f = (data & failedMask) | (e.data & failedMask);
			Nat a = min(shifts() + e.shifts(), shiftsMask >> shiftsShift);
			Nat b = min(skipped() + e.skipped(), skippedMask);

			return InfoErrors(f | (a << shiftsShift) | b);
		}

		InfoErrors &InfoErrors::operator +=(InfoErrors e) {
			// We're just an int, so this is fairly reasonable.
			*this = *this + e;
			return *this;
		}

		static inline Nat value(InfoErrors e) {
			if (!e.success())
				return failedMask;
			else
				return e.shifts() + e.skipped();
		}

		Bool InfoErrors::operator <(InfoErrors e) const {
			return value(*this) < value(e);
		}

		Bool InfoErrors::operator ==(InfoErrors e) const {
			return value(*this) == value(e);
		}

		Bool InfoErrors::operator !=(InfoErrors e) const {
			return value(*this) != value(e);
		}

		InfoErrors infoFailure() {
			return InfoErrors(failedMask);
		}

		InfoErrors infoSuccess() {
			return InfoErrors(0);
		}

		InfoErrors infoShifts(Nat shifts) {
			shifts = min(shifts, shiftsMask >> shiftsShift);
			return InfoErrors((shifts << shiftsShift) & shiftsMask);
		}

		InfoErrors infoSkipped(Nat skipped) {
			skipped = min(skipped, skippedMask);
			return InfoErrors(skipped & skippedMask);
		}

		Nat InfoErrors::getData(InfoErrors e) {
			return e.data;
		}

		InfoErrors InfoErrors::fromData(Nat v) {
			return InfoErrors(v);
		}

		StrBuf &operator <<(StrBuf &to, InfoErrors e) {
			if (e.success()) {
				if (!e.any()) {
					to << L"success";
				} else {
					to << e.skipped() << (e.skipped() == 1 ? L" char" : L" chars") << L" skipped, ";
					to << e.shifts() << (e.shifts() == 1 ? L" correction" : L" corrections");
				}
			} else {
				to << L"failure";
			}

			return to;
		}

	}
}
