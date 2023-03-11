//#include "StdAfx.h"
#include "global.h"
#include "Timer.h"
#include <assert.h>

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

