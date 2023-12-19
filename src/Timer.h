#pragma once

#include <windows.h>

class CTimer
{
public:
   CTimer();
   ~CTimer();

   void Start();
   void Stop();
   [[nodiscard]] double GetMS() const;

private:
   LARGE_INTEGER m_nFreq;
   LARGE_INTEGER m_nT1, m_nT2;
};
