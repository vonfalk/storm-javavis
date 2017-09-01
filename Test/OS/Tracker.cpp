#include "stdafx.h"
#include "Tracker.h"

int Tracker::copies = 0;

Tracker::Tracker(int data) : data(data) {
	copies++;
}

Tracker::Tracker(const Tracker &o) : data(o.data) {
	copies++;
}

Tracker &Tracker::operator =(const Tracker &o) {
	data = o.data;
	return *this;
}

Tracker::~Tracker() {
	copies--;
}

bool Tracker::clear() {
	bool r = copies == 0;
	copies = 0;
	return r;
}
