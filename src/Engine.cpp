#include "StdAfx.h"
#include "Engine.h"

#include <afxwin.h>

#include "Common.h"
#include "Timer.h"
#include "Tournament.h"
#include <fcntl.h>

#ifdef _MSC_VER
#pragma warning(disable : 4244)          
#else
#endif

CEngine::CEngine() : m_sParameterNames(nullptr), m_sParameterValues(nullptr), m_nProcess(nullptr), m_nJob(nullptr)
{
   m_sPath = nullptr;
   m_sName = nullptr;
   m_sUciName = nullptr;
   m_sLBName = nullptr;
   m_sExpectedName = nullptr;
   m_sAuthor = nullptr;
   m_nHash = 0;
   m_bPonder = false;
   m_bOwnBook = false;
   m_nVariant = VARIANT_STD;

   m_nNumParameters = 0;

   m_nBufferSize = 0;
   m_sBuffer[READ] = nullptr;
   m_sBuffer[WRITE] = nullptr;

   m_nPipeRead[READ] = nullptr;
   m_nPipeRead[WRITE] = nullptr;
   m_nPipeWrite[READ] = nullptr;
   m_nPipeWrite[WRITE] = nullptr;

   m_sLastOutput[0] = 0;

   LockInit(&m_nLockEngine, NULL);
}

CEngine::CEngine(const CEngine& other) : CEngine()
{
   CopyConfiguration(other);
}

CEngine& CEngine::operator=(const CEngine& other)
{
   if (this != &other)
   {
      CloseRuntime();
      FreeConfiguration();
      CopyConfiguration(other);
   }
   return *this;
}

CEngine::~CEngine()
{
   CloseRuntime();
   FreeConfiguration();
   LockFree(&m_nLockEngine);
}

namespace
{
char* DuplicateString(const char* value)
{
   if (!value) return nullptr;
   const size_t length = strlen(value) + 1;
   char* copy = new char[length];
   strcpy_s(copy, length, value);
   return copy;
}
}

void CEngine::CopyConfiguration(const CEngine& other)
{
   m_sError.Empty();
   m_sPath = DuplicateString(other.m_sPath);
   m_sName = DuplicateString(other.m_sName);
   m_sUciName = DuplicateString(other.m_sUciName);
   m_sLBName = DuplicateString(other.m_sLBName);
   m_sExpectedName = DuplicateString(other.m_sExpectedName);
   m_sAuthor = DuplicateString(other.m_sAuthor);
   m_nHash = other.m_nHash;
   m_bPonder = other.m_bPonder;
   m_bOwnBook = other.m_bOwnBook;
   m_nVariant = other.m_nVariant;
   m_nNumParameters = other.m_nNumParameters;

   if (m_nNumParameters > 0)
   {
      m_sParameterNames = new char* [m_nNumParameters]{};
      m_sParameterValues = new char* [m_nNumParameters]{};
      for (int i = 0; i < m_nNumParameters; ++i)
      {
         m_sParameterNames[i] = DuplicateString(other.m_sParameterNames[i]);
         m_sParameterValues[i] = DuplicateString(other.m_sParameterValues[i]);
      }
   }
}

void CEngine::FreeConfiguration()
{
   delete[] m_sPath;
   delete[] m_sName;
   delete[] m_sUciName;
   delete[] m_sLBName;
   delete[] m_sExpectedName;
   delete[] m_sAuthor;
   for (int i = 0; i < m_nNumParameters; ++i)
   {
      delete[] m_sParameterNames[i];
      delete[] m_sParameterValues[i];
   }
   delete[] m_sParameterNames;
   delete[] m_sParameterValues;
   m_sPath = m_sName = m_sUciName = m_sLBName = m_sExpectedName = m_sAuthor = nullptr;
   m_sParameterNames = m_sParameterValues = nullptr;
   m_nNumParameters = 0;
}

void CEngine::CloseRuntime()
{
   delete[] m_sBuffer[READ];
   m_sBuffer[READ] = nullptr;
   m_nBufferSize = 0;

   for (HANDLE* handle : { &m_nPipeRead[READ], &m_nPipeRead[WRITE], &m_nPipeWrite[READ], &m_nPipeWrite[WRITE], &m_nProcess,
      &m_nJob })
   {
      if (*handle)
      {
         CloseHandle(*handle);
         *handle = nullptr;
      }
   }
}

bool CEngine::Init()
{
   m_sError.Empty();
   if (!m_sPath || m_sPath[0] == 0) return false;

   CloseRuntime();

   m_sBuffer[READ] = new char[IO_BUFFER + 1];
   SECURITY_ATTRIBUTES saAttr;

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = nullptr;

   if (!CreatePipe(&m_nPipeRead[READ], &m_nPipeRead[WRITE], &saAttr, 0))
   {
      TRACE("Stdout pipe creation failed\n");
      m_sError.Format(_T("Could not create the output pipe for %s"), m_sPath);
      CloseRuntime();
      return false;
   }

   if (!SetHandleInformation(m_nPipeRead[READ], HANDLE_FLAG_INHERIT, 0))
   {
      m_sError.Format(_T("Could not protect the parent output pipe handle for %s"), m_sPath);
      CloseRuntime();
      return false;
   }
   DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
   if (!SetNamedPipeHandleState(m_nPipeRead[READ], &mode, nullptr, nullptr))
   {
      TRACE("SetNamedPipeHandleState failed\n");
      m_sError.Format(_T("Could not configure the output pipe for %s"), m_sPath);
      CloseRuntime();
      return false;
   }

   if (!CreatePipe(&m_nPipeWrite[READ], &m_nPipeWrite[WRITE], &saAttr, 0))
   {
      TRACE("Stdin pipe creation failed\n");
      m_sError.Format(_T("Could not create the input pipe for %s"), m_sPath);
      CloseRuntime();
      return false;
   }

   if (!SetHandleInformation(m_nPipeWrite[WRITE], HANDLE_FLAG_INHERIT, 0))
   {
      m_sError.Format(_T("Could not protect the parent input pipe handle for %s"), m_sPath);
      CloseRuntime();
      return false;
   }

   if (const BOOL fSuccess = CreateChildProcess(m_sPath, m_nPipeWrite[READ], m_nPipeRead[WRITE], &m_nProcess, &m_nJob);
      !fSuccess)
   {
      TRACE("Create process failed\n");
      m_sError.Format(_T("Could not load process %s (%lu)"), m_sPath, GetLastError());
      CloseRuntime();
      return false;
   }

   CloseHandle(m_nPipeWrite[READ]);
   m_nPipeWrite[READ] = nullptr;
   CloseHandle(m_nPipeRead[WRITE]);
   m_nPipeRead[WRITE] = nullptr;

   m_nBufferSize = 0;

   CString sLine;
   Send("uci");

   do
   {
      if (!GetLine(&sLine))
      {
         m_sError.Format(_T("Timed out or lost the process while waiting for the UCI name from %s"), m_sPath);
         Quit();
         return false;
      }
   } while (sLine.Find("id name") == -1);
   long a, b, c;
   ProcessInput(sLine, &a, &b, &c);

   do
   {
      if (!GetLine(&sLine))
      {
         m_sError.Format(_T("Timed out or lost the process while waiting for uciok from %s"), m_sPath);
         Quit();
         return false;
      }
      long i, j;
      ProcessInput(sLine, &i, &j, &c);
   } while (sLine.Find("uciok") == -1);

   if (!m_sUciName || !*m_sUciName)
   {
      m_sError.Format(_T("Engine %s did not provide a usable UCI id name"), m_sPath);
      Quit();
      return false;
   }

   if (m_sExpectedName && _stricmp(m_sExpectedName, m_sUciName) != 0)
   {
      m_sError.Format(_T("Engine identity mismatch for %s: expected \"%s\", received \"%s\""), m_sPath, m_sExpectedName,
         m_sUciName ? m_sUciName : "(missing)");
      Log("%s", static_cast<const char*>(m_sError));
      Quit();
      return false;
   }

   Send("setoption name Hash value %d", m_nHash ? m_nHash : 1);
   Send("setoption name Ponder value %s", m_bPonder ? "true" : "false");
   Send("setoption name OwnBook value %s", m_bOwnBook ? "true" : "false");
   if (m_nVariant == VARIANT_960)
   {
      Send("setoption name UCI_Chess960 value true");
   }

   for (int i = 0; i < m_nNumParameters; i++)
   {
      Send("setoption name %s value %s", m_sParameterNames[i], m_sParameterValues[i]);
   }

   Send("isready");

   do
   {
      if (!GetLine(&sLine))
      {
         m_sError.Format(_T("Timed out or lost the process while waiting for readyok from %s"), m_sPath);
         Quit();
         return false;
      }
   } while (sLine.Find("readyok") == -1);

   return true;
}

void CEngine::NewGame()
{
   CString sLine;

   Send("ucinewgame");

   Send("isready");
   do
   {
      if (!GetLine(&sLine)) return;
   } while (sLine.Find("readyok") == -1);
}

CString CEngine::Search(const CString& sStartingPositionFEN, const CString& sMoves, const int nTC, const long nWhiteTime, const long nBlackTime,
   const long nWhiteInc, const long nBlackInc, const long nTimeOut, long* nDepth, long* nNPS, long* nScore,
   CTimer* t)
{
   CString sLine;
   CString s;

   CString startpos;
   if (!sStartingPositionFEN.Compare(_T("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0")))
   {
      startpos.AppendFormat(_T("startpos"));
   }
   else
   {
      startpos.AppendFormat(_T("fen %s"), sStartingPositionFEN.GetString());
   }

   if (sMoves.GetLength() > 0)
   {
      s.AppendFormat(_T("position %s moves%s"), startpos.GetString(), sMoves.GetString());
      Send(s);
   }
   else
   {
      s.AppendFormat(_T("position %s"), startpos.GetString());
      Send(s);
   }
   if (nTC == TC_FIXED_TPM)
   {
      s.Format(_T("go movetime %ld"), nWhiteTime);
   }
   else if (nTC == TC_BLITZ)
   {
      s.Format(_T("go wtime %ld btime %ld winc %ld binc %ld"), nWhiteTime, nBlackTime, nWhiteInc, nBlackInc);
   }
   else
   {
      s.Format(_T("go movestogo %ld wtime %ld btime %ld"), nWhiteInc, nWhiteTime, nBlackTime);
   }

   t->Start();
   Send(s);

   do
   {
      const double elapsed = t->GetMS();
      const DWORD remaining = static_cast<DWORD>(MAX(1.0, nTimeOut + 1000.0 - elapsed));
      if (!GetLine(&sLine, remaining))
      {
         t->Stop();
         return CString();
      }
      t->Stop();
      ProcessInput(sLine, nDepth, nNPS, nScore);
   } while (sLine.Find("bestmove") == -1
      && t->GetMS() <= nTimeOut + 1000
      );
   if (t->GetMS() > nTimeOut + 1000)
   {
      ASSERT(false);
   }

   strncpy_s(m_sLastOutput, sLine.GetBuffer(), 99);
   m_sLastOutput[99] = 0;

   return sLine;

}

void CEngine::Stop()
{
   Send("stop");
}

void CEngine::Quit()
{
   Lock(&m_nLockEngine);

   if (!m_nProcess)
   {
      Unlock(&m_nLockEngine);
      return;
   }

   if (m_nPipeWrite[WRITE])
   {
      Send("stop");
      Send("quit");
   }

   if (const DWORD r = WaitForSingleObject(m_nProcess, 30000); r == WAIT_TIMEOUT)
   {
      if (const BOOL b = TerminateProcess(m_nProcess, 1); !b)
      {
         if (const DWORD e = GetLastError(); e != 5)
         {
            char err[100];
            sprintf_s(err, static_cast<const char*>("Terminate %s errored: %lu"), m_sName ? m_sName : "engine", e);
            Log("%s", err);
         }
      }
   }

   CloseRuntime();

   Unlock(&m_nLockEngine);
}

void CEngine::ProcessInput(const CString& sLine, long* nDepth, long* nNPS, long* nScore)
{
   CStringArray sWords;

   if (sLine.GetLength() == 0) return;

   GetWords(sLine, &sWords);
   if (sWords.IsEmpty()) return;

   if (!sWords.GetAt(0).CompareNoCase(_T("info")))
   {
      bool principalVariation = true;
      bool boundedScore = false;
      for (int i = 1; i < sWords.GetCount(); ++i)
      {
         if (!sWords.GetAt(i).CompareNoCase(_T("multipv")) && i + 1 < sWords.GetCount())
            principalVariation = atol(sWords.GetAt(i + 1)) == 1;
         else if (!sWords.GetAt(i).CompareNoCase(_T("lowerbound"))
            || !sWords.GetAt(i).CompareNoCase(_T("upperbound")))
            boundedScore = true;
      }

      for (int i = 1; i + 1 < sWords.GetCount(); i++)
      {
         if (const CString& s = sWords.GetAt(i); !s.CompareNoCase(_T("depth")))
         {
            CString s2 = sWords.GetAt(i + 1);
            *nDepth = atol(s2.GetBuffer());
         }
         else if (!s.CompareNoCase(_T("nps")))
         {
            CString s2 = sWords.GetAt(i + 1);
            *nNPS = atol(s2.GetBuffer());
         }
         else if (principalVariation && !boundedScore && !s.CompareNoCase(_T("score"))
            && i + 2 < sWords.GetCount() && !sWords.GetAt(i + 1).CompareNoCase(_T("cp")))
         {
            CString s2 = sWords.GetAt(i + 2);
            *nScore = atol(s2.GetBuffer());
         }
         else if (principalVariation && !boundedScore && !s.CompareNoCase(_T("score"))
            && i + 2 < sWords.GetCount() && !sWords.GetAt(i + 1).CompareNoCase(_T("mate")))
         {
            CString s2 = sWords.GetAt(i + 2);
            *nScore = atol(s2.GetBuffer()) * 2;
            if (*nScore < 0) *nScore = -30000 - *nScore;
            else *nScore = 30000 - *nScore;
         }
      }
   }
   else if (!sWords.GetAt(0).CompareNoCase(_T("id")))
   {
      if (sWords.GetCount() >= 3 && !sWords.GetAt(1).CompareNoCase(_T("name")))
      {
         int nLen = 0;
         for (int i = 2; i < sWords.GetSize(); i++)
         {
            nLen += sWords.GetAt(i).GetLength() + 1;
         }
         delete[] m_sUciName;
         m_sUciName = new char[nLen + 1];
         m_sUciName[0] = 0;
         for (int i = 0; i < sWords.GetSize() - 2; i++)
         {
            CString s = sWords.GetAt(2 + i);
            strcat_s(m_sUciName, nLen + 1, s.GetBuffer());
            strcat_s(m_sUciName, nLen + 1, " ");
         }
         m_sUciName[nLen - 1] = 0;

         if (m_sLBName)
         {
            delete[] m_sName;
            m_sName = new char[strlen(m_sLBName) + 1];
            strcpy_s(m_sName, strlen(m_sLBName) + 1, m_sLBName);
         }
         else
         {
            delete[] m_sName;
            m_sName = DuplicateString(m_sUciName);
         }
      }
      else if (sWords.GetCount() >= 3 && !sWords.GetAt(1).CompareNoCase(_T("author")))
      {
         CString s = sWords.GetAt(2);
         delete[] m_sAuthor;
         m_sAuthor = new char[s.GetLength() + 1];
         strcpy_s(m_sAuthor, s.GetLength() + 1, s.GetBuffer());
      }
   }
   else if (!sWords.GetAt(0).CompareNoCase(_T("option")))
   {
   }

   sWords.RemoveAll();
}

int CEngine::GetWords(const CString& sLine, CStringArray* sWords)
{
   int nNumWords = 0;
   int pos = 0;
   CString sWord = sLine.Tokenize(_T(" "), pos);
   while (pos != -1)
   {
      sWords->Add(sWord);
      nNumWords++;
      sWord = sLine.Tokenize(_T(" "), pos);
   }

   return nNumWords;
}

void CEngine::Send(const char format[], ...)
{
   char buf[4096];
   va_list arg_list;
   va_start(arg_list, format);
   _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, format, arg_list);
   va_end(arg_list);

   const CString sLine(buf);
   Send(sLine);
}

void CEngine::Send(CString sLine)
{
   if (sLine.IsEmpty() || !m_nPipeWrite[WRITE]) return;

   DWORD dwWritten;

   if (sLine[sLine.GetLength() - 1] != '\n')
   {
      sLine.Append(_T("\n"));
   }

   Log("-->(%s) %s", m_sName ? m_sName : "engine", sLine.GetString());

   if (!WriteFile(m_nPipeWrite[WRITE], sLine, sLine.GetLength(), &dwWritten, nullptr))
   {
   }

}

bool CEngine::GetLine(CString* sLine, const DWORD nTimeoutMs)
{
   if (!sLine) return false;
   sLine->Empty();
   const ULONGLONG deadline = GetTickCount64() + nTimeoutMs;
   while (!IsDataWaiting())
   {
      UpdateBuffer();
      if (IsDataWaiting()) break;
      if (!m_nProcess || WaitForSingleObject(m_nProcess, 0) == WAIT_OBJECT_0 || GetTickCount64() >= deadline)
         return false;
      Sleep(5);
   }

   if (!m_nBufferSize) return false;

   const auto p = static_cast<char*>(memchr(m_sBuffer[READ], '\n', m_nBufferSize));
   if (!p) return false;

   const int nLen = p - m_sBuffer[READ] + 1;
   ASSERT(nLen > 0);
   int n = nLen;
   if (m_sBuffer[READ][nLen - 1] == '\r' || m_sBuffer[READ][nLen - 1] == '\n') n--;
   if (nLen >= 2 && (m_sBuffer[READ][nLen - 2] == '\r' || m_sBuffer[READ][nLen - 2] == '\n')) n--;

   char logbuf[IO_BUFFER + 1];
   strncpy_s(logbuf, m_sBuffer[READ], nLen);
   logbuf[nLen - 1] = 0;
   Log("<--(%s) %s", m_sName ? m_sName : "engine", logbuf);

   sLine->SetString(m_sBuffer[READ], n);
   for (int i = 0; i < m_nBufferSize - nLen; i++)
   {
      m_sBuffer[READ][i] = m_sBuffer[READ][i + nLen];
   }
   m_nBufferSize -= nLen;
   m_sBuffer[READ][m_nBufferSize] = 0;
   return true;
}

bool CEngine::IsDataWaiting() const
{
   if (!memchr(m_sBuffer[READ], '\n', m_nBufferSize))
      return false;
   return true;
}

bool CEngine::UpdateBuffer()
{
   const int nSize = IO_BUFFER - m_nBufferSize;
   if (!m_nPipeRead[READ] || nSize <= 0) return false;

   DWORD dwRead = 0;

   if (!ReadFile(m_nPipeRead[READ], m_sBuffer[READ] + m_nBufferSize, nSize, &dwRead, nullptr))
   {
      return false;
   }
   ASSERT(dwRead > 0);

   m_nBufferSize += dwRead;
   ASSERT(m_nBufferSize <= IO_BUFFER);

   return true;
}
