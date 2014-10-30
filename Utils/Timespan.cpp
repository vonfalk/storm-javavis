#include "StdAfx.h"
#include "Timespan.h"

#include <sstream>
#include <iomanip>

const Timespan Timespan::zero(Timespan::ms(0));

Timespan::Timespan() {
	time = 0;
}

Timespan::Timespan(int64 micros) {
	this->time = micros;
}

Timespan::Timespan(const void *from) {
	time = *((int64 *)from);
}


void Timespan::save(void *to) const {
	*((int64 *)to) = time;
}

void Timespan::output(std::wostream &output) const {
	int64 time = ::abs(this->time);
	if (time < 1000) {
		output << this->time << L" us";
	} else if (time < 1000 * 1000) {
		output << this->time / 1000 << L" ms";
	} else {
		output << std::fixed <<std::setprecision(2) << this->time / 1000000.0 << L" s";
	}
}

String Timespan::format() const {
	std::wstringstream output;

	int64 seconds = ::abs(this->time) / (1000 * 1000);
	int64 minutes = seconds / 60;
	seconds = seconds % 60;
	output << minutes << L":" <<std::setw(2) << std::setfill(L'0') << seconds;
	return output.str();
}

Timespan abs(const Timespan &time) {
	return Timespan(::abs(time.micros()));
}
