#include "StdAfx.h"
#include "Timer.h"

CTimer::CTimer()
{
   QueryPerformanceFrequency(&m_nFreq);
   m_nT1.QuadPart = 0;
   m_nT2.QuadPart = 0;
}

CTimer::~CTimer()
= default;

void CTimer::Start()
{
   QueryPerformanceCounter(&m_nT1);
}

void CTimer::Stop()
{
   QueryPerformanceCounter(&m_nT2);
}

double CTimer::GetMS() const
{
   if (m_nT2.QuadPart == 0)
   {
      LARGE_INTEGER i;
      QueryPerformanceCounter(&i);
      return static_cast<double>(i.QuadPart - m_nT1.QuadPart) * 1000.0 / static_cast<double>(m_nFreq.QuadPart);
   }
   return static_cast<double>(m_nT2.QuadPart - m_nT1.QuadPart) * 1000.0 / static_cast<double>(m_nFreq.QuadPart);
}
