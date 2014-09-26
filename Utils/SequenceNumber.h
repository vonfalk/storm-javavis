#pragma once

namespace util {

	class SequenceNumber {
	public:
		SequenceNumber(int startAt = 1);
		~SequenceNumber();

		operator int();

		int get();
		int peek() const;
	private:
		int currentNumber;
		int startNumber;
	};

};