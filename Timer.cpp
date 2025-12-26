#include "stdafx.h"
#include "global.h"
#include "Timer.h"
#include <assert.h>

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>	// OutputDebugString()
#else
    #include <time.h>
#endif

CTimer g_Timer;

CTimer::CTimer()
{
#ifdef _WIN32
	QueryPerformanceFrequency(&m_PerformanceFrequency);
#else
    // clock_gettime()
    m_PerformanceFrequency.QuadPart = 1000000000;
#endif
}


double CTimer::GetAbsoluteTime() const
{
	assert(m_PerformanceFrequency.QuadPart);

	LARGE_INTEGER b;

#ifdef _WIN32
	QueryPerformanceCounter(&b);
#else
    struct timespec ts;
    // Use CLOCK_MONOTONIC for time-interval measurements
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        return -1; // Failure

    // Combine seconds and nanoseconds into a 64-bit integer (nanoseconds)
    b.QuadPart = ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif

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
