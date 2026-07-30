#include "StdAfx.h"
#include "Common.h"

#include <bcrypt.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#ifdef _MSC_VER
#pragma warning(disable : 4146)           
#else
#endif

FILE* fLog = nullptr;
char sLogPath[1024];

bool g_bLogging = false;
bool g_bDumpIllegalMoves = true;
bool g_bFullPGN = false;
bool g_bDarkMode = false;

namespace
{
BOOL CALLBACK ApplyDarkModeToChild(const HWND child, const LPARAM enabled)
{
   if (enabled)
   {
      wchar_t className[16] = {};
      GetClassNameW(child, className, _countof(className));
      const LONG_PTR buttonType = GetWindowLongPtr(child, GWL_STYLE) & BS_TYPEMASK;
      const bool useDialogTextColors = wcscmp(className, L"Button") == 0
         && (buttonType == BS_GROUPBOX || buttonType == BS_RADIOBUTTON
            || buttonType == BS_AUTORADIOBUTTON);

      // The dark Explorer theme paints group-box and radio-button labels black,
      // bypassing the dialog's WM_CTLCOLOR text color. Let those controls use
      // the dialog colors while retaining dark themed checkboxes and buttons.
      SetWindowTheme(child, useDialogTextColors ? L"" : L"DarkMode_Explorer",
         useDialogTextColors ? L"" : nullptr);
   }
   else
   {
      SetWindowTheme(child, nullptr, nullptr);
   }
   SendMessage(child, WM_THEMECHANGED, 0, 0);
   return TRUE;
}
}

void ApplyDarkModeToWindow(const HWND window, const bool enabled)
{
   if (!IsWindow(window)) return;

   const BOOL useDarkMode = enabled ? TRUE : FALSE;
   constexpr DWORD immersiveDarkMode = 20;
   constexpr DWORD immersiveDarkModeBefore20H1 = 19;
   if (FAILED(DwmSetWindowAttribute(window, immersiveDarkMode, &useDarkMode, sizeof(useDarkMode))))
      DwmSetWindowAttribute(window, immersiveDarkModeBefore20H1, &useDarkMode, sizeof(useDarkMode));

   SetWindowTheme(window, enabled ? L"DarkMode_Explorer" : nullptr, nullptr);
   EnumChildWindows(window, ApplyDarkModeToChild, enabled ? 1 : 0);
   RedrawWindow(window, nullptr, nullptr,
      RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
}

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
   if (!sPath) return;
   sprintf_s(sLogPath, "%s\\LittleBlitzer.log", sPath);
   delete[] sPath;

   fLog = fopen(sLogPath, "wt");
   if (fLog) fclose(fLog);
}

void Log(const char format[], ...)
{
   if (!g_bLogging) return;
   static std::mutex logMutex;
   const std::lock_guard<std::mutex> lock(logMutex);
   if (!fLog)
   {
      ResetLog();
   }
   char buf[16 * 1024];
   va_list arg_list;
   va_start(arg_list, format);
   _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, format, arg_list);
   va_end(arg_list);

   if (buf[0] == 0) return;

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

BOOL CreateChildProcess(const char* sPath, const HANDLE hIn, const HANDLE hOut, HANDLE* hProcess, HANDLE* hJob)
{
   if (!sPath || !*sPath || !hProcess || !hJob) return FALSE;
   *hProcess = nullptr;
   *hJob = nullptr;

   char szCmdline[1024];
   if (sprintf_s(szCmdline, "\"%s\"", sPath) < 0) return FALSE;
   PROCESS_INFORMATION piProcInfo{};
   STARTUPINFOEXA siStartInfo{};

   siStartInfo.StartupInfo.cb = sizeof(STARTUPINFOEXA);
   siStartInfo.StartupInfo.hStdError = hOut;
   siStartInfo.StartupInfo.hStdOutput = hOut;
   siStartInfo.StartupInfo.hStdInput = hIn;
   siStartInfo.StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

   char sCWD[1024];
   const char* backslash = strrchr(sPath, '\\');
   const char* slash = strrchr(sPath, '/');
   const char* separator = backslash;
   if (slash && (!separator || slash > separator)) separator = slash;
   if (separator)
   {
      const size_t cwdLength = static_cast<size_t>(separator - sPath);
      if (cwdLength == 0 || cwdLength >= sizeof(sCWD)) return FALSE;
      strncpy_s(sCWD, sPath, cwdLength);
      sCWD[cwdLength] = 0;
   }
   else if (!GetCurrentDirectoryA(sizeof(sCWD), sCWD))
   {
      return FALSE;
   }

   SIZE_T attributeBytes = 0;
   InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
   std::vector<unsigned char> attributeStorage(attributeBytes);
   siStartInfo.lpAttributeList = reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
   if (!InitializeProcThreadAttributeList(siStartInfo.lpAttributeList, 1, 0, &attributeBytes))
      return FALSE;

   HANDLE inheritedHandles[] = { hIn, hOut };
   if (!UpdateProcThreadAttribute(siStartInfo.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
      inheritedHandles, sizeof(inheritedHandles), nullptr, nullptr))
   {
      DeleteProcThreadAttributeList(siStartInfo.lpAttributeList);
      return FALSE;
   }

   const BOOL bFuncRetn = CreateProcessA(nullptr,
      szCmdline,
      nullptr,
      nullptr,
      TRUE,
      CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
      nullptr,
      sCWD,
      &siStartInfo.StartupInfo,
      &piProcInfo);

   DeleteProcThreadAttributeList(siStartInfo.lpAttributeList);

   if (bFuncRetn == 0)
   {
      const int n = GetLastError();
      TRACE("CreateProcess failed: %d\n", n);
      return FALSE;
   }

   HANDLE job = CreateJobObjectA(nullptr, nullptr);
   if (job)
   {
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
      limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
      if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))
         || !AssignProcessToJobObject(job, piProcInfo.hProcess))
      {
         CloseHandle(job);
         job = nullptr;
      }
   }

   if (ResumeThread(piProcInfo.hThread) == static_cast<DWORD>(-1))
   {
      TerminateProcess(piProcInfo.hProcess, 1);
      WaitForSingleObject(piProcInfo.hProcess, 5000);
      CloseHandle(piProcInfo.hThread);
      CloseHandle(piProcInfo.hProcess);
      if (job) CloseHandle(job);
      return FALSE;
   }

   CloseHandle(piProcInfo.hThread);
   *hProcess = piProcInfo.hProcess;
   *hJob = job;
   return TRUE;
}

bool GetFileSha256(const char* path, CStringA* digest)
{
   if (!path || !*path || !digest) return false;
   digest->Empty();

   const HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
   if (file == INVALID_HANDLE_VALUE) return false;

   BCRYPT_ALG_HANDLE algorithm = nullptr;
   BCRYPT_HASH_HANDLE hash = nullptr;
   DWORD objectBytes = 0;
   DWORD hashBytes = 0;
   DWORD copied = 0;
   bool ok = false;
   std::vector<unsigned char> object;
   std::vector<unsigned char> value;
   std::vector<unsigned char> buffer;
   char encoded[65]{};
   static constexpr char hex[] = "0123456789ABCDEF";

   if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
      goto cleanup;
   if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes),
      &copied, 0) < 0)
      goto cleanup;
   if (BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashBytes), sizeof(hashBytes), &copied,
      0) < 0)
      goto cleanup;

   object.resize(objectBytes);
   value.resize(hashBytes);
   buffer.resize(64 * 1024);

   if (BCryptCreateHash(algorithm, &hash, object.data(), objectBytes, nullptr, 0, 0) < 0)
      goto cleanup;

   while (true)
   {
      DWORD bytesRead = 0;
      if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr))
         goto cleanup;
      if (bytesRead == 0) break;
      if (BCryptHashData(hash, buffer.data(), bytesRead, 0) < 0)
         goto cleanup;
   }
   if (BCryptFinishHash(hash, value.data(), hashBytes, 0) < 0)
      goto cleanup;

   if (hashBytes != 32) goto cleanup;
   for (DWORD i = 0; i < hashBytes; ++i)
   {
      encoded[i * 2] = hex[value[i] >> 4];
      encoded[i * 2 + 1] = hex[value[i] & 0x0F];
   }
   *digest = encoded;
   ok = true;

cleanup:
   if (hash) BCryptDestroyHash(hash);
   if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
   CloseHandle(file);
   return ok;
}

wchar_t* ConvertCharToWCharBecauseMSDontProvideOne(const char* str)
{
   if (!str) return nullptr;
   size_t length = 0;
   mbstowcs_s(&length, nullptr, 0, str, 0);
   wchar_t* result = new wchar_t[length];
   mbstowcs_s(&length, result, length, str, _TRUNCATE);
   return result;
}
