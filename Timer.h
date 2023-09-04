#pragma once

#include <string>
#define _AMD64_
#include <profileapi.h>

// very simple CPU timer class
class CTimer
{
public:
	CTimer();

	// @return in seconds
	double GetAbsoluteTime() const;

private: // -------------------------------------------

	LARGE_INTEGER		m_PerformanceFrequency;
};

extern CTimer g_Timer;

class CScopedCPUTimerLog
{
public:
	CScopedCPUTimerLog(const char* InText);

	~CScopedCPUTimerLog();

	// must not be 0
	const char* Text;
	// can be 0
	std::wstring Name;
	//
	double StartTime;
};

/*
class CScopedCPUTimer
{
public:
	CScopedCPUTimer(const char* InText, const std::wstring InName)
		: Text(InText)
		, Name(InName)
	{
		assert(InText);
		StartTime = g_Timer.GetAbsoluteTime();
	}

	~CScopedCPUTimer();

	// must not be 0
	const char* Text;
	// can be 0
	std::wstring Name;
	//
	double StartTime;
};

// measure until scope end and dump to log, useful for loading, not so much each frame, can only be once in that scope (more doesn't make much sense)
// some memory allocations (can be optimized)
// cascades don't subtract from each other (yet)
#define SCOPED_CPU_TIMER_LOG(Text) CScopedCPUTimerLog ScopedCPUTimerLog(#Text, TEXT(""))
#define SCOPED_CPU_TIMER_LOG2(Text, WName) CScopedCPUTimerLog ScopedCPUTimerLog(#Text, WName)

#define SCOPED_CPU_TIMER(Text) CScopedCPUTimer ScopedCPUTimer##Text(#Text, TEXT(""))


*/