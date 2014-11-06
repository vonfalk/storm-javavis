#pragma once

class CriticalSection {
public:
	CriticalSection();
	~CriticalSection();

	class Section {
	public:
		Section(CriticalSection &section);
		~Section();
	private:
		CriticalSection *section;
	};

	typedef Section L; //Shorthand name
private:
	CRITICAL_SECTION criticalSection;
};

typedef CriticalSection Lock; //Alternate name
