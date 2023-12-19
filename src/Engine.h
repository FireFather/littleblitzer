#pragma once

#include "Timer.h"
#include <afxstr.h>
#include <afxcoll.h>

#define IO_BUFFER		16384
#define MAX_PARMS		100
class CEngine
{
public:
   CEngine();
   ~CEngine();

   CRITICAL_SECTION m_nLockEngine;

   char* m_sPath;
   char* m_sName;
   char* m_sLBName;
   char* m_sAuthor;
   long m_nHash;
   bool m_bPonder;
   bool m_bOwnBook;
   int m_nVariant;
   char** m_sParameterNames;
   char** m_sParameterValues;
   int m_nNumParameters;
   char m_sLastOutput[100];

   HANDLE m_nPipeRead[2];
   HANDLE m_nPipeWrite[2];
   int m_nBufferSize;
   char* m_sBuffer[2];
   HANDLE m_nProcess;

   void Send(CString sLine);
   void Send(const char format[], ...);
   void GetLine(CString* sLine);
   [[nodiscard]] bool IsDataWaiting() const;
   bool UpdateBuffer();
   void ProcessInput(const CString& sLine, long* nDepth, long* nNPS, long* nScore);
   static int GetWords(const CString& sLine, CStringArray* sWords);

   bool Init();
   void NewGame();
   CString Search(const CString& sStartingPositionFEN, const CString& sMoves, int nTC, long nWhiteTime, long nBlackTime,
      long nWhiteInc, long nBlackInc, long nTimeOut, long* nDepth, long* nNPS, long* nScore, CTimer* t);
   void Stop();
   void Quit();
};
