#include "stdafx.h"
#include "global.h"
#include "Timer.h"
#include <assert.h>

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>	// OutputDebugString()
#endif

CTimer g_Timer;

CTimer::CTimer()
{
#ifdef _WIN32
	QueryPerformanceFrequency(&m_PerformanceFrequency);
#else
    assert(0);
#endif
}


double CTimer::GetAbsoluteTime() const
{
	assert(m_PerformanceFrequency.QuadPart);

#ifdef _WIN32
	LARGE_INTEGER b;

	QueryPerformanceCounter(&b);

	return (double)(b.QuadPart) / (double)m_PerformanceFrequency.QuadPart;
#else
    assert(0);
#endif
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
