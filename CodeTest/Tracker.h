#pragma once

class Tracker {
public:
	Tracker(int data);
	Tracker(const Tracker &o);
	~Tracker();

	// Carrying some data.
	int data;

	// Clear it (returns null if no live instances).
	static bool clear();

private:
	static int copies;

};

