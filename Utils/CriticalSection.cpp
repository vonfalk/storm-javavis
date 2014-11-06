#include "StdAfx.h"
#include "CriticalSection.h"

CriticalSection::CriticalSection() {
	InitializeCriticalSection(&criticalSection);
}

CriticalSection::~CriticalSection() {
	DeleteCriticalSection(&criticalSection);
}

CriticalSection::Section::Section(CriticalSection &section) {
	EnterCriticalSection(&section.criticalSection);
	this->section = &section;
}

CriticalSection::Section::~Section() {
	LeaveCriticalSection(&section->criticalSection);
}
