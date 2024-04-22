//#include "StdAfx.h"
#include "global.h"
#include "Timer.h"
#include <assert.h>
#include <windows.h>	// OutputDebugString()
#undef max
#undef min

CTimer g_Timer;

CTimer::CTimer()
{
	QueryPerformanceFrequency(&m_PerformanceFrequency);
}


double CTimer::GetAbsoluteTime() const
{
	assert(m_PerformanceFrequency.QuadPart);

	LARGE_INTEGER b;

	QueryPerformanceCounter(&b);

	return (double)(b.QuadPart) / (double)m_PerformanceFrequency.QuadPart;
}



CScopedCPUTimerLog::CScopedCPUTimerLog(const char* InText)
	: Text(InText)
{
	assert(InText);
	StartTime = g_Timer.GetAbsoluteTime();
}

CScopedCPUTimerLog::~CScopedCPUTimerLog()
{
	double EndTime = g_Timer.GetAbsoluteTime();

	float TimeDelta = (float)(EndTime - StartTime);

	char str[1024];

	sprintf_s(str, sizeof(str) / sizeof(str[0]), " %*.1f ms ", 6, TimeDelta * 1000);		// 6 spaces (see trick here: http://www.cplusplus.com/reference/cstdio/printf/)
	OutputDebugStringA(str);

	sprintf_s(str, sizeof(str) / sizeof(str[0]), "  SCOPED_CPU_TIMER_LOG %s", Text);
	OutputDebugStringA(str);

	//		if(!Name.empty())
	//		{
	//			OutputDebugStringA(": ");
	//			OutputDebugStringW(Name.c_str());
	//		}

	OutputDebugStringA("\n");
}
