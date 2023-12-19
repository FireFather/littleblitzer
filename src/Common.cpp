#include "StdAfx.h"
#include "Common.h"

#ifdef _MSC_VER
#pragma warning(disable : 4146)           
#else
#endif

FILE* fLog = nullptr;
char sLogPath[1024];

bool g_bLogging = false;
bool g_bDumpIllegalMoves = true;
bool g_bFullPGN = false;

char* GetFilePath(char* sFilePath)
{
   const char* pLast = strrchr(sFilePath, '\\');
   char* sPath = nullptr;
   if (pLast)
   {
      sPath = new char[pLast - sFilePath + 1];
      strncpy(sPath, sFilePath, pLast - sFilePath);
      sPath[pLast - sFilePath] = 0;
   }
   return sPath;
}

wchar_t* GetFilePath(wchar_t* sFilePath)
{
   const wchar_t* pLast = wcsrchr(sFilePath, '\\');
   wchar_t* sPath = nullptr;
   if (pLast)
   {
      sPath = new wchar_t[pLast - sFilePath + 1];
      wcsncpy(sPath, sFilePath, pLast - sFilePath);
      sPath[pLast - sFilePath] = 0;
   }
   return sPath;
}

void ResetLog()
{
   char sFile[1024];
   GetModuleFileNameA(nullptr, sFile, 1024);
   char* sPath = GetFilePath(sFile);
   sprintf(sLogPath, "%s\\LittleBlitzer.log", sPath);
   delete sPath;

   fLog = fopen(sLogPath, "wt");
   fclose(fLog);
}

void Log(const char format[], ...)
{
   if (!g_bLogging) return;
   if (!fLog)
   {
      ResetLog();
   }
   char buf[16 * 1024];
   va_list arg_list;
   va_start(arg_list, format);
   vsprintf(buf, format, arg_list);
   va_end(arg_list);

   fLog = fopen(sLogPath, "at");
   if (!fLog) return;
   if (buf[strlen(buf) - 1] == '\n' || buf[strlen(buf) - 1] == '\r')
      fprintf(fLog, "%s", buf);
   else
      fprintf(fLog, "%s\n", buf);
   fclose(fLog);
}

unsigned int LSB(const BitBoard b)
{
   ASSERT(b);

   unsigned long index;
   if (b & 0xffffffff)
   {
      _BitScanForward(&index, b & 0xffffffff);
      return index;
   }
   _BitScanForward(&index, b >> 32);
   return index + 32;

}

unsigned int MSB(const BitBoard b)
{
   ASSERT(b);

   unsigned long index;
   if (b >> 32)
   {
      _BitScanReverse(&index, b >> 32);
      return index + 32;
   }
   _BitScanReverse(&index, b & 0xffffffff);
   return index;
}

unsigned int CountBits(const BitBoard bb)
{
   unsigned int w = static_cast<unsigned int>(bb >> 32), v = static_cast<unsigned int>(bb);
   v = v - (v >> 1 & 0x55555555);
   w = w - (w >> 1 & 0x55555555);
   v = (v & 0x33333333) + (v >> 2 & 0x33333333);
   w = (w & 0x33333333) + (w >> 2 & 0x33333333);
   v = v + (v >> 4) & 0x0F0F0F0F;
   w = w + (w >> 4) & 0x0F0F0F0F;
   v = (v + w) * 0x01010101 >> 24;
   return v;
}

int RemoveBit(BitBoard& bb)
{
   BitBoard tbb;
   const int sq = GetBit(tbb = bb & -bb);
   bb ^= tbb;
   return sq;
}

BOOL CreateChildProcess(const char* sPath, const HANDLE hIn, const HANDLE hOut, HANDLE* hProcess)
{
   char szCmdline[1024];
   strcpy(szCmdline, sPath);
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;

   ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

   ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = hOut;
   siStartInfo.hStdOutput = hOut;
   siStartInfo.hStdInput = hIn;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

   const char* p = strrchr(szCmdline, '\\');
   ASSERT(p);
   char sCWD[1024];
   ASSERT(p - szCmdline > 0);
   strncpy(sCWD, szCmdline, p - szCmdline);
   sCWD[p - szCmdline] = 0;

   wchar_t szCmdline_W[1024];
   wchar_t sCWD_W[1024];
   size_t dumb;
   mbstowcs_s(&dumb, szCmdline_W, strlen(szCmdline) + 1, szCmdline, 1024);
   mbstowcs_s(&dumb, sCWD_W, strlen(sCWD) + 1, sCWD, 1024);

   const BOOL bFuncRetn = CreateProcess(nullptr,
      szCmdline,
      nullptr,
      nullptr,
      TRUE,
      CREATE_NO_WINDOW,
      nullptr,
      sCWD,
      &siStartInfo,
      &piProcInfo);

   *hProcess = piProcInfo.hProcess;

   if (bFuncRetn == 0)
   {
      int n = GetLastError();
      TRACE("CreateProcess failed: %d\n", n);
   }
   else
   {
      CloseHandle(piProcInfo.hThread);
   }

   return bFuncRetn;
}

wchar_t* ConvertCharToWCharBecauseMSDontProvideOne(const char* str)
{
   wchar_t w[1024];
   size_t dumb;
   mbstowcs_s(&dumb, w, strlen(str) + 1, str, 1024);
   return w;
}
