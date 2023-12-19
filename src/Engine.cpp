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

CEngine::CEngine() : m_sParameterNames(nullptr), m_sParameterValues(nullptr), m_nProcess(nullptr)
{
   m_sPath = nullptr;
   m_sName = nullptr;
   m_sLBName = nullptr;
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

CEngine::~CEngine()
= default;

bool CEngine::Init()
{
   if (m_sPath[0] == 0) return false;

   m_sBuffer[READ] = new char[IO_BUFFER + 1];
   SECURITY_ATTRIBUTES saAttr;

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = nullptr;

   if (!CreatePipe(&m_nPipeRead[READ], &m_nPipeRead[WRITE], &saAttr, 0))
   {
      TRACE("Stdout pipe creation failed\n");
      CString s;
      s.Format(_T("Count not create read pipe for %s"), m_sPath);
      MessageBox(nullptr, s, _T("Error"), MB_OK);
      return false;
   }

   SetHandleInformation(m_nPipeRead[READ], HANDLE_FLAG_INHERIT, 0);
   DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
   if (!SetNamedPipeHandleState(m_nPipeRead[READ], &mode, nullptr, nullptr))
   {
      TRACE("SetNamedPipeHandleState failed\n");
      CString s;
      s.Format(_T("Count not set named pipe for %s"), m_sPath);
      MessageBox(nullptr, s, _T("Error"), MB_OK);
      return false;
   }

   if (!CreatePipe(&m_nPipeWrite[READ], &m_nPipeWrite[WRITE], &saAttr, 0))
   {
      TRACE("Stdin pipe creation failed\n");
      CString s;
      s.Format(_T("Count not create write pipe for %s"), m_sPath);
      MessageBox(nullptr, s, _T("Error"), MB_OK);
      return false;
   }

   SetHandleInformation(m_nPipeWrite[WRITE], HANDLE_FLAG_INHERIT, 0);

   if (const BOOL fSuccess = CreateChildProcess(m_sPath, m_nPipeWrite[READ], m_nPipeRead[WRITE], &m_nProcess); !fSuccess)
   {
      TRACE("Create process failed\n");
      CString s;
      s.Format(_T("Could not load process %s (%lu)"), m_sPath, GetLastError());
      MessageBox(nullptr, s, _T("Error"), MB_OK);
      return false;
   }

   m_nBufferSize = 0;

   CString sLine;
   Send("uci");

   do
   {
      GetLine(&sLine);
   } while (sLine.Find("id name") == -1);
   long a, b, c;
   ProcessInput(sLine, &a, &b, &c);

   do
   {
      GetLine(&sLine);
      long i, j;
      ProcessInput(sLine, &i, &j, &c);
   } while (sLine.Find("uciok") == -1);

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
      GetLine(&sLine);
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
      GetLine(&sLine);
      Sleep(10);
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
      startpos.AppendFormat(_T("fen %s"), sStartingPositionFEN);
   }

   if (sMoves.GetLength() > 0)
   {
      s.AppendFormat(_T("position %s moves%s"), startpos, sMoves);
      Send(s);
   }
   else
   {
      s.AppendFormat(_T("position %s"), startpos);
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
      GetLine(&sLine);
      t->Stop();
      ProcessInput(sLine, nDepth, nNPS, nScore);
   } while (sLine.Find("bestmove") == -1
      && t->GetMS() <= nTimeOut + 1000
      );
   if (t->GetMS() > nTimeOut + 1000)
   {
      ASSERT(false);
   }

   strncpy(m_sLastOutput, sLine.GetBuffer(), 100);
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

   Send("stop");
   Send("quit");

   if (const DWORD r = WaitForSingleObject(m_nProcess, 30000); r == WAIT_TIMEOUT)
   {
      if (const BOOL b = TerminateProcess(m_nProcess, 1); !b)
      {
         if (const DWORD e = GetLastError(); e != 5)
         {
            char err[100];
            sprintf(err, static_cast<const char*>("Terminate %s errored: %lu"), m_sName, e);
            AfxMessageBox(err);
         }
      }
   }

   delete m_sBuffer[READ];
   delete m_sName;
   delete m_sAuthor;

   m_sBuffer[READ] = nullptr;
   m_sName = nullptr;
   m_sAuthor = nullptr;

   CloseHandle(m_nPipeWrite[WRITE]);
   CloseHandle(m_nPipeRead[READ]);

   Unlock(&m_nLockEngine);
}

void CEngine::ProcessInput(const CString& sLine, long* nDepth, long* nNPS, long* nScore)
{
   CStringArray sWords;

   if (sLine.GetLength() == 0) return;

   int nNumWords = GetWords(sLine, &sWords);

   if (!sWords.GetAt(0).CompareNoCase(_T("info")))
   {
      for (int i = 1; i < sWords.GetCount(); i++)
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
         else if (!s.CompareNoCase(_T("cp")))
         {
            CString s2 = sWords.GetAt(i + 1);
            *nScore = atol(s2.GetBuffer());
         }
         else if (!s.CompareNoCase(_T("mate")))
         {
            CString s2 = sWords.GetAt(i + 1);
            *nScore = atol(s2.GetBuffer()) * 2;
            if (*nScore < 0) *nScore = -30000 - *nScore;
            else *nScore = 30000 - *nScore;
         }
      }
   }
   else if (!sWords.GetAt(0).CompareNoCase(_T("id")))
   {
      if (!sWords.GetAt(1).CompareNoCase(_T("name")))
      {
         int nLen = 0;
         for (int i = 2; i < sWords.GetSize(); i++)
         {
            nLen += sWords.GetAt(i).GetLength() + 1;
         }
         if (m_sLBName)
         {
            m_sName = new char[strlen(m_sLBName) + 1];
            strncpy(m_sName, m_sLBName, strlen(m_sLBName));
            m_sName[strlen(m_sLBName)] = 0;
         }
         else
         {
            m_sName = new char[nLen + 1];
            m_sName[0] = 0;
            for (int i = 0; i < sWords.GetSize() - 2; i++)
            {
               CString s = sWords.GetAt(2 + i);
               strcat(m_sName, s.GetBuffer());
               strcat(m_sName, " ");
            }
            m_sName[nLen - 1] = 0;
         }
      }
      else if (!sWords.GetAt(1).CompareNoCase(_T("author")))
      {
         CString s = sWords.GetAt(2);
         m_sAuthor = new char[s.GetLength() + 1];
         strcpy(m_sAuthor, s.GetBuffer());
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
   vsprintf(buf, format, arg_list);
   va_end(arg_list);

   const CString sLine(buf);
   Send(sLine);
}

void CEngine::Send(CString sLine)
{
   DWORD dwWritten;

   if (sLine[sLine.GetLength() - 1] != '\n')
   {
      sLine.Append(_T("\n"));
   }

   Log("-->(%s) %s", m_sName, sLine);

   if (!WriteFile(m_nPipeWrite[WRITE], sLine, sLine.GetLength(), &dwWritten, nullptr))
   {
   }

}

void CEngine::GetLine(CString* sLine)
{
   while (!IsDataWaiting()
      && UpdateBuffer()) {
   }

   if (!m_nBufferSize) return;

   const auto p = static_cast<char*>(memchr(m_sBuffer[READ], '\n', m_nBufferSize));
   if (!p) return;

   const int nLen = p - m_sBuffer[READ] + 1;
   ASSERT(nLen > 0);
   int n = nLen;
   if (m_sBuffer[READ][nLen - 1] == '\r' || m_sBuffer[READ][nLen - 1] == '\n') n--;
   if (m_sBuffer[READ][nLen - 2] == '\r' || m_sBuffer[READ][nLen - 2] == '\n') n--;

   char logbuf[10240];
   strncpy(logbuf, m_sBuffer[READ], nLen);
   logbuf[nLen - 1] = 0;
   Log("<--(%s) %s", m_sName, logbuf);

   sLine->SetString(m_sBuffer[READ], n);
   for (int i = 0; i < m_nBufferSize - nLen; i++)
   {
      m_sBuffer[READ][i] = m_sBuffer[READ][i + nLen];
   }
   m_nBufferSize -= nLen;
   m_sBuffer[READ][m_nBufferSize] = 0;
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

   DWORD dwRead;

   if (!ReadFile(m_nPipeRead[READ], m_sBuffer[READ] + m_nBufferSize, nSize, &dwRead, nullptr))
   {
      if (dwRead == 0)
      {
         Sleep(10);
      }
      return false;
   }
   ASSERT(dwRead > 0);

   m_nBufferSize += dwRead;
   ASSERT(m_nBufferSize <= IO_BUFFER);

   return true;
}
