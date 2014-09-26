#include "StdAfx.h"
#include "CriticalSection.h"

util::CriticalSection::CriticalSection() {
	InitializeCriticalSection(&criticalSection);
}

util::CriticalSection::~CriticalSection() {
	DeleteCriticalSection(&criticalSection);
}

util::CriticalSection::Section::Section(CriticalSection &section) {
	EnterCriticalSection(&section.criticalSection);
	this->section = &section;
}

util::CriticalSection::Section::~Section() {
	LeaveCriticalSection(&section->criticalSection);
}